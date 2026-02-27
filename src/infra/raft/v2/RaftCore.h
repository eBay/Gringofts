/************************************************************************
Copyright 2019-2020 eBay Inc.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
**************************************************************************/

#ifndef SRC_INFRA_RAFT_V2_RAFTCORE_H_
#define SRC_INFRA_RAFT_V2_RAFTCORE_H_

#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <sstream>
#include <absl/strings/str_join.h>

#include <INIReader.h>

#include "../../util/ClusterInfo.h"
#include "../../util/RandomUtil.h"
#include "../../util/SecretKeyFactory.h"
#include "../../util/TestPointProcessor.h"
#include "../../util/TimeUtil.h"
#include "../../util/Util.h"
#include "../DoubleBuffer.h"
#include "../generated/raft.pb.h"
#include "../storage/InMemoryLog.h"
#include "../storage/Log.h"
#include "../storage/SegmentLog.h"
#include "../RaftConstants.h"
#include "../RaftInterface.h"
#include "../StreamingService.h"
#include "RaftService.h"

namespace gringofts {
namespace raft {
namespace v2 {

struct Peer {
  uint64_t mId = 0;

  /// ip:port
  std::string mAddress;

  /**
   * Set to true if this server has responded to our RV_req/RPV_req
   * in the current term, false otherwise.
   */
  bool mRequestVoteDone = false;

  /**
   * Set to true if this server has granted us its vote for this term.
   */
  bool mHaveVote = false;

  /**
   * Return the largest entry index for which this server is known to share the
   * same entries up to and including this entry with our log.
   * This is used for advancing the leader's commitIndex.
   *
   * Monotonically increases within a term.
   * Only used when leader.
   */
  uint64_t mMatchIndex = 0;

  /**
   * The index of the next entry to send to the follower. Only used when
   * leader. Minimum value is 1.
   */
  uint64_t mNextIndex = 1;

  /**
   * Indicates that the leader and the follower aren't necessarily
   * synchronized. The leader should not send large amounts of data (with
   * many log entries or large chunks of a snapshot file) to the follower
   * while this flag is true. For example, the follower might have been
   * disconnected, or the leader might not know where the follower's log
   * diverges from its own. It's better to sync up using small RPCs like
   * heartbeats, then begin/resume sending bulk data after receiving an
   * acknowledgment.
   *
   * Only used when leader.
   */
  bool mSuppressBulkData = true;

  /**
   * Last sent time of AE_req/RV_req/RPV_req by Leader/Candidate/PreCandidate
   */
  uint64_t mLastRequestTimeInNano = 0;

  /**
   * As a switch used by Leader/Candidate/PreCandidate to determine
   * whether next AE_req/RV_req/RPV_req is ready to send.
   */
  uint64_t mNextRequestTimeInNano = 0;

  /**
   * Last response time of AE_resp/RV_resp/RPV_resp, used by Leader
   * to ensure its leadership. Leader should step down if can not
   * communicate with majority within election timeout.
   */
  uint64_t mLastResponseTimeInNano = 0;
};

class RaftCore : public RaftInterface {
 public:
  struct ClusterMemberShip {
    std::map<uint64_t, Peer> mPeers;
    std::map<uint32_t, RaftRole> mInitialRoles;
    MemberInfo mSelfInfo;
    std::map<uint64_t, std::unique_ptr<RaftClient>> mClients;
    std::set<uint32_t> mOldMembers;
    std::set<uint32_t> mNewMembers;
    std::set<uint32_t> mInsyncLearners;
    void clear() {
      SPDLOG_INFO("Abandon Current Membership");
      mPeers.clear();
      mInitialRoles.clear();
      mSelfInfo = {};
      mClients.clear();
      mOldMembers.clear();
      mNewMembers.clear();
      mInsyncLearners.clear();
    }
    void printCluster() const {
      std::stringstream ss;
      ss << "========== PRINT CLUSTER MEMBERSHIP ==========" << std::endl;
      ss << "Self: " << mSelfInfo.mId << "; ";
      std::set<uint32_t> peers;
      for (auto &peer : mPeers) {
        peers.insert(peer.first);
      }
      ss << "Peers: " << absl::StrJoin(peers, ",") << "; ";
      std::vector<std::string> roles;
      for (const auto &node : mInitialRoles) {
        roles.emplace_back(roleString(node.second));
      }
      ss << "InitRoles: " << absl::StrJoin(roles, ",") << "; ";
      ss << "OldMembers: " << absl::StrJoin(mOldMembers, ",") << "; ";
      ss << "NewMembers: " << absl::StrJoin(mNewMembers, ",") << "; ";
      ss << "InsyncLearners: " << absl::StrJoin(mInsyncLearners, ",");
      SPDLOG_INFO(ss.str());
    }
  };
  RaftCore(const char *configPath,
      const NodeId &selfId,
      const ClusterInfo &clusterInfo,
      std::shared_ptr<DNSResolver> dnsResolver,
      RaftRole role = RaftRole::Follower,
      std::shared_ptr<SecretKeyFactoryInterface> secretKeyFactory = std::make_shared<SecretKeyFactoryDefault>());

  ~RaftCore() override;

  RaftRole getRaftRole() const override { return mRaftRole; }
  uint64_t getCommitIndex() const override { return mCommitIndex; }
  uint64_t getCurrentTerm() const override { return mLog->getCurrentTerm(); }
  uint64_t getFirstLogIndex() const override { return mLog->getFirstLogIndex(); }
  uint64_t getBeginLogIndex() const override { return mBeginIndex; }
  uint64_t getLastLogIndex() const override { return mLog->getLastLogIndex(); }

  std::optional<uint64_t> getLeaderHint() const override {
    uint64_t leaderId = mLeaderId;
    return leaderId != 0 ? std::optional<uint64_t>(leaderId) : std::nullopt;
  }
  std::vector<MemberInfo> getClusterMembers() const override {
    std::vector<MemberInfo> cluster;
    auto members = mClusterMembers->constCurrent();
    cluster.push_back(members->mSelfInfo);
    for (const auto &[id, p] : members->mPeers) {
      cluster.push_back({id, p.mAddress});
    }
    /// guarantee a unique sort order
    std::sort(cluster.begin(), cluster.end(),
              [](const MemberInfo &info1, const MemberInfo &info2) { return info1.mId < info2.mId; });
    return cluster;
  }

  bool isLearner(uint64_t id) const override {
    auto members = mClusterMembers->constCurrent();
    return members->mInitialRoles.at(id) == RaftRole::Learner;
  }

  bool isPreFollower(uint64_t id) const override {
    auto members = mClusterMembers->constCurrent();
    return members->mInitialRoles.at(id) == RaftRole::PreFollower;
  }

  bool isVoter(uint64_t id) const override {
    auto members = mClusterMembers->constCurrent();
    return members->mInitialRoles.at(id) == RaftRole::Follower;
  }

  bool getEntry(uint64_t index, LogEntry *entry) const override {
    if (mRaftRole != RaftRole::Syncer) {
      // for syncer mode, all logs actually has been
      // committed in other cluster
      assert(index <= mCommitIndex);
    }
    return mLog->getEntry(index, entry);
  }

  uint64_t getEntries(uint64_t startIndex,
                      uint64_t size,
                      std::vector<LogEntry> *entries) const override {
    assert(startIndex + size - 1 <= mCommitIndex);
    return mLog->getEntries(startIndex, mMaxLenInBytes, size, entries);
  }

  void enqueueClientRequests(ClientRequests clientRequests) override {
    auto event = std::make_shared<ClientRequestsEvent>();

    event->mType = RaftEventBase::Type::ClientRequest;
    event->mPayload = std::move(clientRequests);

    mClientRequestsQueue.enqueue(std::move(event));
  }

  void enqueueSyncRequest(SyncRequest syncRequest) override {
    auto event = std::make_shared<SyncRequestsEvent>();

    event->mType = RaftEventBase::Type::SyncRequest;
    event->mPayload = std::move(syncRequest);

    mClientRequestsQueue.enqueue(std::move(event));
  }

  void enqueueReconfigureRequest(ReconfigureRequest reconfigureRequest) override {
    auto event = std::make_shared<ReconfigureRequestEvent>();

    event->mType = RaftEventBase::Type::ReconfigureRequest;
    event->mPayload = std::move(reconfigureRequest);

    mClientRequestsQueue.enqueue(std::move(event));
  }

  void truncatePrefix(uint64_t firstIndexKept) override {
    assert(firstIndexKept <= mCommitIndex);
    return mLog->truncatePrefix(firstIndexKept);
  }

  /// return leader commit index
  uint64_t getMemberOffsets(std::vector<MemberOffsetInfo> *) const override;

 private:
  /// init
  void initConfigurableVars(const INIReader &iniReader);
  void initClusterConf(const ClusterInfo &clusterInfo, const NodeId &selfId,
      std::shared_ptr<ClusterMemberShip> members);
  void initStorage(const INIReader &iniReader, std::shared_ptr<SecretKeyFactoryInterface> secretKeyFactory);
  void initService(const INIReader &iniReader);

  /// thread function of mRaftLoop
  void raftLoopMain();

  /// dequeue and handle a message from event queue,
  /// including mAeRvQueue and mClientRequestsQueue.
  void receiveMessage();

  /// send AE_req
  void appendEntries();

  /// send RV_req/RPV_req
  void requestVote();

  /// receive AE_req, reply AE_resp
  grpc::Status handleAppendEntriesRequest(const AppendEntries::Request &request,
                                  AppendEntries::Response *response);

  /// receive AE_resp
  void handleAppendEntriesResponse(const AppendEntries::Response &response);

  /// receive RV_req/RPV_req, reply RV_resp/RPV_resp
  grpc::Status handleRequestVoteRequest(const RequestVote::Request &request,
                                RequestVote::Response *response);

  /// receive RV_resp/RPV_resp
  void handleRequestVoteResponse(const RequestVote::Response &response);

  /// receive ClientRequests
  void handleClientRequests(ClientRequests clientRequests);

  /// receive syncRequest
  void handleSyncRequest(SyncRequest syncRequest);

  /// receive reconfigureRequest
  void handleReconfigureRequest(ReconfigureRequest reconfigureRequest);

  void advanceCommitIndex();

  void becomeLeader();

  /// preCandidate -> Candidate
  void becomeCandidate();

  /// transition from Follower/Candidate to Candidate of next term
  void electionTimeout();

  /// Leader should step down if can not communicate
  /// with majority within election timeout.
  void leadershipTimeout();

  void configurationSwitch();

  void stepDown(uint64_t newTerm);

  /// be careful that precedence of '>>' is less than '+'
  static uint64_t getMajorityNumber(uint64_t totalNum) { return (totalNum >> 1) + 1; }

  void checkSelfNeedExit() {
    auto members = mClusterMembers->constCurrent();
    if (members->mSelfInfo.mId == kBadID) {
      // if this node is not in the configure list, exit
      SPDLOG_INFO("Exit for configure dont not contain may selfId");
      assert(0);
    }
  }

  uint64_t getMajorityIndexofVoters(const std::set<uint32_t> &nodeIds) {
    auto members = mClusterMembers->constCurrent();
    std::vector<uint64_t> indices;
    for (auto &id : nodeIds) {
      if (!isVoter(id)) {
        continue;
      }
      if (members->mSelfInfo.mId == id) {
        indices.push_back(mLog->getLastLogIndex());
      } else {
        auto it = members->mPeers.find(id);
        assert(it != members->mPeers.end());
        indices.push_back(it->second.mMatchIndex);
      }
    }
    std::sort(indices.begin(), indices.end(),
                [](uint64_t x, uint64_t y) { return x > y; });
    return indices[indices.size() >> 1];
  }

  // only used in in-sync learner feature
  uint64_t getMajorityIndexofLearners(const std::set<uint32_t> &nodeIds) {
    auto members = mClusterMembers->constCurrent();
    std::vector<uint64_t> indices;
    for (auto &id : nodeIds) {
      if (!isLearner(id)) {
        continue;
      }
      auto it = members->mPeers.find(id);
      assert(it != members->mPeers.end());
      indices.push_back(it->second.mMatchIndex);
    }
    std::sort(indices.begin(), indices.end(),
                [](uint64_t x, uint64_t y) { return x > y; });
    return indices[indices.size() >> 1];
  }

  // return current majority index, support both single-cluster and joint-cluster
  uint64_t getMajorityIndex() {
    auto members = mClusterMembers->constCurrent();
    if (members->mOldMembers.empty() || members->mNewMembers.empty()) {
      /// calculate the largest entry stored on a quorum of servers
      /// work for single-server cluster as well
      std::set<uint32_t> nodeIds;
      for (auto &p : members->mPeers) {
        auto &peer = p.second;
        nodeIds.insert(peer.mId);
      }
      nodeIds.insert(members->mSelfInfo.mId);
      if (members->mInsyncLearners.empty()) {
        return getMajorityIndexofVoters(nodeIds);
      } else {
        // take the majority index of in-sync learners into consideration while advancing commitIndex
        return std::min(getMajorityIndexofVoters(nodeIds), getMajorityIndexofLearners(members->mInsyncLearners));
      }
    } else {
      /// calculate the largest entry stored on joint-consensus
      /// work for the status of Cold & Cnew co-exist
      return std::min(getMajorityIndexofVoters(members->mOldMembers), getMajorityIndexofVoters(members->mNewMembers));
    }
  }

  // count the vote number for current node in the nodeIds, return whether reach quorum size
  // note that current node may not be in the nodeIds set
  bool checkQuorumVoteofInput(const std::set<uint32_t> &nodeIds) {
    auto members = mClusterMembers->constCurrent();
    uint64_t voteNum = 0;
    uint64_t nonVoterNum = 0;
    for (auto &id : nodeIds) {
      // accept the RV response from a server, which is recoginized as a prefollower
      // align the rule: for RV request, always believe what received is the latest configuration
      if (isLearner(id)) {
        ++nonVoterNum;
        continue;
      }
      if (members->mSelfInfo.mId == id) {
        ++voteNum;
      } else {
        auto it = members->mPeers.find(id);
        assert(it != members->mPeers.end());
        if (it->second.mHaveVote) {
          ++voteNum;
        }
      }
    }
    return voteNum >= getMajorityNumber(nodeIds.size() - nonVoterNum);
  }

  // check whether pass prevote or vote, support both single-cluster and joint-cluster
  bool checkVotePass() {
    auto members = mClusterMembers->constCurrent();
    if (members->mOldMembers.empty() || members->mNewMembers.empty()) {
      std::set<uint32_t> nodeIds;
      for (auto &p : members->mPeers) {
        auto &peer = p.second;
        nodeIds.insert(peer.mId);
      }
      nodeIds.insert(members->mSelfInfo.mId);
      return checkQuorumVoteofInput(nodeIds);
    } else {
      // joint consensus status, Cold & Cnew
      return checkQuorumVoteofInput(members->mOldMembers) && checkQuorumVoteofInput(members->mNewMembers);
    }
  }

  uint64_t getMajorityTimeElapseofInput(const std::set<uint32_t> &nodeIds) {
    auto members = mClusterMembers->constCurrent();
    std::vector<uint64_t> timePoints;
    auto nowInNano = TimeUtil::currentTimeInNanos();
    for (auto &id : nodeIds) {
      if (!isVoter(id)) {
        continue;
      }
      if (members->mSelfInfo.mId == id) {
        timePoints.push_back(nowInNano);
      } else {
        auto it = members->mPeers.find(id);
        assert(it != members->mPeers.end());
        timePoints.push_back(it->second.mLastResponseTimeInNano);
      }
    }
    std::sort(timePoints.begin(), timePoints.end(),
                [](uint64_t x, uint64_t y) { return x > y; });
    return nowInNano - timePoints[timePoints.size() >> 1];
  }

  // return current majority timeelapse, support both single-cluster and joint-cluster
  uint64_t getMajorityTimeElapse() {
    auto members = mClusterMembers->constCurrent();
    if (members->mOldMembers.empty() || members->mNewMembers.empty()) {
      std::set<uint32_t> nodeIds;
      for (auto &p : members->mPeers) {
        auto &peer = p.second;
        nodeIds.insert(peer.mId);
      }
      nodeIds.insert(members->mSelfInfo.mId);
      return getMajorityTimeElapseofInput(nodeIds);
    } else {
      return std::max(getMajorityTimeElapseofInput(members->mOldMembers),
          getMajorityTimeElapseofInput(members->mNewMembers));
    }
  }

  uint64_t termOfLogEntryAt(uint64_t index) const {
    uint64_t term;
    assert(mLog->getTerm(index, &term));
    return term;
  }

  /// update election time point, after
  /// 1) receive AE_req from current leader,
  /// 2) grant vote to candidate
  /// 3) become Follower
  /// 4) become Candidate
  void updateElectionTimePoint() {
    auto timeIntervalInNano = RandomUtil::randomRange(mMinElectionTimeoutInMillis * 1000 * 1000,
                                                      mMaxElectionTimeoutInMillis * 1000 * 1000);
    mElectionTimePointInNano = TimeUtil::currentTimeInNanos() + timeIntervalInNano;
  }

  static std::string roleString(const RaftRole &role) {
    switch (role) {
      case RaftRole::Leader: return "Leader";
      case RaftRole::Follower: return "Follower";
      case RaftRole::Candidate: return "Candidate";
      case RaftRole::Syncer: return "Syncer";
      case RaftRole::PreCandidate: return "PreCandidate";
      case RaftRole::Learner: return "Learner";
      case RaftRole::PreFollower: return "PreFollower";
      default: return "";
    }
  }

  std::string selfId() const {
    auto members = mClusterMembers->constCurrent();
    return roleString(mRaftRole) + " " + std::to_string(members->mSelfInfo.mId);
  }

  // should only call by leader
  std::string otherId(MemberId id) const {
    assert(mRaftRole == RaftRole::Leader);
    auto members = mClusterMembers->constCurrent();
    return roleString(members->mInitialRoles.at(id)) + " " + std::to_string(id);
  }

  /// for some reason, print status.
  void printStatus(const std::string &reason) const;

  /// metrics
  void printMetrics(const AppendEntries::Metrics &metrics) const;

  /**
   * configurable vars
   */
  /// heart beat interval that leader will wait before sending a heartbeat to follower
  uint64_t mHeartBeatIntervalInMillis = RaftConstants::kHeartBeatIntervalInMillis;
  /// the minimum/maximum timeout follower will wait before starting a new election
  uint64_t mMinElectionTimeoutInMillis = RaftConstants::kMinElectionTimeoutInMillis;
  uint64_t mMaxElectionTimeoutInMillis = RaftConstants::kMaxElectionTimeoutInMillis;
  /// the timeout for RPCs
  uint64_t mRpcAppendEntriesTimeoutInMillis = RaftConstants::AppendEntries::kRpcTimeoutInMillis;
  uint64_t mRpcRequestVoteTimeoutInMillis = RaftConstants::RequestVote::kRpcTimeoutInMillis;
  /// for getEntries()
  uint64_t mMaxBatchSize = 2000;
  uint64_t mMaxLenInBytes = 5000000;
  /// for handleAppendEntriesResponse()
  uint64_t mMaxDecrStep = 2000;
  /// for printStatus()
  uint64_t mMaxTailedEntryNum = 5;
  bool mPreVoteEnabled = false;

  /**
   * raft state
   */
  /// 0 should not be used as mId
  static constexpr uint64_t kBadID = 0;

  std::atomic<uint64_t> mLeaderId = kBadID;
  /// if now > election time point, incr current term, convert to candidate
  uint64_t mElectionTimePointInNano = 0;

  std::atomic<uint64_t> mCommitIndex = 0;

  /// for cluster 0, is 1
  /// otherwise, this is the first index this cluster start to process
  uint64_t mBeginIndex = 1;

  RaftRole mRaftRole = RaftRole::Follower;

  /// entries, currentTerm, voteFor
  std::unique_ptr<storage::Log> mLog;

  /// pending client requests, list of <index, handle>
  std::list<std::pair<uint64_t, RequestHandle *>> mPendingClientRequests;

  /**
   * threading model
   */
  EventQueue mAeRvQueue;
  EventQueue mClientRequestsQueue;

  /// raft main loop
  std::atomic<bool> running = true;
  std::thread mRaftLoop;

  /// raft service: server and clients
  std::unique_ptr<RaftServer> mServer;

  // this data member should put after mAeRvQueue
  // because raftclient mainloop depened on mAeRvQueue
  // so raftclient should be destruct before mAeRvQueue
  std::shared_ptr<DoubleBuffer<ClusterMemberShip>> mClusterMembers;

  /// streaming service
  bool mStreamingSvcEnabled = false;
  uint64_t mStreamingPort;
  std::unique_ptr<StreamingService> mStreamingService;
  std::optional<TlsConf> mTlsConfOpt;
  std::shared_ptr<DNSResolver> mDNSResolver;

  /**
   * monitor
   */
  santiago::MetricsCenter::GaugeType mLeadershipGauge;

  /// after restart, Leader/Follower will recover commitIndex from 0,
  /// ignore the flip from 0 to avoid confusing metrics
  santiago::MetricsCenter::CounterType mCommitIndexCounter;

  /// UT
  RaftCore(
      const char *configPath,
      const NodeId &myNodeId,
      const ClusterInfo &clusterInfo,
      TestPointProcessor *processor);

  TestPointProcessor *mTPProcessor = nullptr;
  friend class ClusterTestUtil;
  FRIEND_TEST(RaftCoreTest, BasicTest);
};

}  /// namespace v2
}  /// namespace raft
}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_V2_RAFTCORE_H_

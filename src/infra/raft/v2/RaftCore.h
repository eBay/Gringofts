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

#include <INIReader.h>

#include "../../monitor/MonitorTypes.h"
#include "../../util/RandomUtil.h"
#include "../../util/TestPointProcessor.h"
#include "../../util/TimeUtil.h"
#include "../../util/Util.h"
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
   * Set to true if this server has responded to our RV_req
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
   * Last sent time of AE_req/RV_req by Leader/Candidate
   */
  uint64_t mLastRequestTimeInNano = 0;

  /**
   * As a switch used by Leader/Candidate to determine
   * whether next AE_req/RV_req is ready to send.
   */
  uint64_t mNextRequestTimeInNano = 0;

  /**
   * Last response time of AE_resp/RV_resp, used by Leader
   * to ensure its leadership. Leader should step down if can not
   * communicate with majority within election timeout.
   */
  uint64_t mLastResponseTimeInNano = 0;
};

class RaftCore : public RaftInterface {
 public:
  RaftCore(const char *configPath, std::optional<std::string> clusterConfOpt);

  ~RaftCore() override;

  RaftRole getRaftRole() const override { return mRaftRole; }
  uint64_t getCommitIndex() const override { return mCommitIndex; }
  uint64_t getCurrentTerm() const override { return mLog->getCurrentTerm(); }
  uint64_t getFirstLogIndex() const override { return mLog->getFirstLogIndex(); }
  uint64_t getLastLogIndex() const override { return mLog->getLastLogIndex(); }

  std::optional<uint64_t> getLeaderHint() const override {
    uint64_t leaderId = mLeaderId;
    return leaderId != 0 ? std::optional<uint64_t>(leaderId) : std::nullopt;
  }

  bool getEntry(uint64_t index, LogEntry *entry) const override {
    assert(index <= mCommitIndex);
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

  void truncatePrefix(uint64_t firstIndexKept) override {
    assert(firstIndexKept <= mCommitIndex);
    return mLog->truncatePrefix(firstIndexKept);
  }

 private:
  /// init
  void initConfigurableVars(const INIReader &iniReader);
  void initClusterConf(const INIReader &iniReader, std::optional<std::string> clusterConfOpt);
  void initStorage(const INIReader &iniReader);
  void initService(const INIReader &iniReader);

  /// given <peerId, host, port>, determine whether it is itself.
  using IsSelf = std::function<bool(uint64_t peerId,
                                    const std::string &host, const std::string &port)>;

  void initClusterConfImpl(const std::string &clusterConf, IsSelf isSelf);

  /// thread function of mRaftLoop
  void raftLoopMain();

  /// dequeue and handle a message from event queue,
  /// including mAeRvQueue and mClientRequestsQueue.
  void receiveMessage();

  /// send AE_req
  void appendEntries();

  /// send RV_req
  void requestVote();

  /// receive AE_req, reply AE_resp
  void handleAppendEntriesRequest(const AppendEntries::Request &request,
                                  AppendEntries::Response *response);

  /// receive AE_resp
  void handleAppendEntriesResponse(const AppendEntries::Response &response);

  /// receive RV_req, reply RV_resp
  void handleRequestVoteRequest(const RequestVote::Request &request,
                                RequestVote::Response *response);

  /// receive RV_resp
  void handleRequestVoteResponse(const RequestVote::Response &response);

  /// receive ClientRequests
  void handleClientRequests(ClientRequests clientRequests);

  void advanceCommitIndex();

  void becomeLeader();

  /// transition from Follower/Candidate to Candidate of next term
  void electionTimeout();

  /// Leader should step down if can not communicate
  /// with majority within election timeout.
  void leadershipTimeout();

  void stepDown(uint64_t newTerm);

  /// be careful that precedence of '>>' is less than '+'
  static uint64_t getMajorityNumber(uint64_t totalNum) { return (totalNum >> 1) + 1; }

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
    auto timeIntervalInNano = RandomUtil::randomRange(RaftConstants::kMinElectionTimeoutInMillis * 1000 * 1000,
                                                      RaftConstants::kMaxElectionTimeoutInMillis * 1000 * 1000);
    mElectionTimePointInNano = TimeUtil::currentTimeInNanos() + timeIntervalInNano;
  }

  std::string selfId() const {
    return (mRaftRole == RaftRole::Leader ? "Leader "
                                          : mRaftRole == RaftRole::Candidate ? "Candidate "
                                                                             : "Follower ") +
                                                                             std::to_string(mSelfInfo.mId);
  }

  /// for some reason, print status.
  void printStatus(const std::string &reason) const;

  /// metrics
  static void printMetrics(const AppendEntries::Metrics &metrics);

  /**
   * configurable vars
   */
  /// for getEntries()
  uint64_t mMaxBatchSize = 2000;
  uint64_t mMaxLenInBytes = 4000000;
  /// for handleAppendEntriesResponse()
  uint64_t mMaxDecrStep = 2000;
  /// for printStatus()
  uint64_t mMaxTailedEntryNum = 5;

  /**
   * raft state
   */
  /// 0 should not be used as mId
  static constexpr uint64_t kBadID = 0;

  std::map<uint64_t, Peer> mPeers;

  MemberInfo mSelfInfo;

  std::atomic<uint64_t> mLeaderId = kBadID;
  /// if now > election time point, incr current term, convert to candidate
  uint64_t mElectionTimePointInNano = 0;

  std::atomic<uint64_t> mCommitIndex = 0;
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
  std::map<uint64_t, std::unique_ptr<RaftClient>> mClients;

  /// streaming service
  std::unique_ptr<StreamingService> mStreamingService;

  /**
   * monitor
   */
  santiago::MetricsCenter::GaugeType mLeadershipGauge;

  /// after restart, Leader/Follower will recover commitIndex from 0,
  /// ignore the flip from 0 to avoid confusing metrics
  santiago::MetricsCenter::CounterType mCommitIndexCounter;

  /// UT
  RaftCore(const char *configPath, TestPointProcessor *processor);
  TestPointProcessor *mTPProcessor = nullptr;
  friend class ClusterTestUtil;
  FRIEND_TEST(RaftCoreTest, BasicTest);
};

}  /// namespace v2
}  /// namespace raft
}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_V2_RAFTCORE_H_

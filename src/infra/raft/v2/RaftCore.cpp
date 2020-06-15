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

#include "RaftCore.h"

#include <assert.h>
#include <limits>
#include <regex>
#include <vector>

namespace gringofts {
namespace raft {
namespace v2 {

RaftCore::RaftCore(const char *configPath, std::optional<std::string> clusterConfOpt) :
    mLeadershipGauge(gringofts::getGauge("leadership_gauge", {{"status", "isLeader"}})),
    mCommitIndexCounter(gringofts::getCounter("committed_log_counter", {{"status", "committed"}})) {
  INIReader iniReader(configPath);
  if (iniReader.ParseError() < 0) {
    SPDLOG_ERROR("Can't load configure file {}, exit", configPath);
    throw std::runtime_error("Can't load config file");
  }

  initConfigurableVars(iniReader);
  initClusterConf(iniReader, clusterConfOpt);
  initStorage(iniReader);
  initService(iniReader);
}

void RaftCore::initConfigurableVars(const INIReader &iniReader) {
  // @formatter:off
  mMaxBatchSize       = iniReader.GetInteger("raft.default", "max.batch.size", 0);
  mMaxLenInBytes      = iniReader.GetInteger("raft.default", "max.len.in.bytes", 0);
  mMaxDecrStep        = iniReader.GetInteger("raft.default", "max.decr.step", 0);
  mMaxTailedEntryNum  = iniReader.GetInteger("raft.default", "max.tailed.entry.num", 0);
  // @formatter:on

  assert(mMaxBatchSize != 0
             && mMaxLenInBytes != 0
             && mMaxDecrStep != 0
             && mMaxTailedEntryNum != 0);

  SPDLOG_INFO("ConfigurableVars: "
              "max.batch.size={}, "
              "max.len.in.bytes={}, "
              "max.decr.step={}, "
              "max.tailed.entry.num={}.",
              mMaxBatchSize, mMaxLenInBytes, mMaxDecrStep, mMaxTailedEntryNum);
}

void RaftCore::initClusterConf(const INIReader &iniReader, std::optional<std::string> clusterConfOpt) {
  if (clusterConfOpt) {
    assert(*clusterConfOpt != "");
    /// N.B.: when using external conf, the assumption is two raft instances will never run on the same host,
    ///       otherwise below logic will break.
    auto hostname = Util::getHostname();

    SPDLOG_INFO("raft cluster conf passed from external, "
                "cluster.conf={}, hostname={}", *clusterConfOpt, hostname);

    initClusterConfImpl(*clusterConfOpt, [hostname](uint64_t peerId,
                                                    const std::string &host,
                                                    const std::string &port) { return host == hostname; });
  } else {
    auto clusterConf = iniReader.Get("raft.default", "cluster.conf", "");
    auto selfId = iniReader.GetInteger("raft.default", "self.id", 0);
    assert(!clusterConf.empty() && selfId > 0);

    SPDLOG_INFO("read raft cluster conf from local, "
                "cluster.conf={}, self.id={}", clusterConf, selfId);

    initClusterConfImpl(clusterConf, [selfId](uint64_t peerId,
                                              const std::string &host,
                                              const std::string &port) { return peerId == selfId; });
  }
}

/**
 * such as: "1@hostname1:port1,2@hostname2:port2,3@hostname3:port3,..."
 *      or: "1@ip1:port1,2@ip2:port2,3@ip3:port3,..."
 *
 * peerId should monotonically increase from 1 without gap
 */
void RaftCore::initClusterConfImpl(const std::string &clusterConf,
                                   IsSelf isSelf) {
  std::regex regex("([0-9]+)@([^:]+):([0-9]+)");
  std::smatch match;

  std::vector<std::string> raftCluster = StrUtil::tokenize(clusterConf, ',');

  for (auto expectId = 1; expectId <= raftCluster.size(); ++expectId) {
    const auto &raftNode = raftCluster[expectId - 1];
    if (!std::regex_search(raftNode, match, regex)) {
      throw std::runtime_error("Bad cluster conf " + raftNode);
    }

    uint64_t peerId = std::stoul(match[1]);
    assert(peerId == expectId);

    std::string host = match[2];
    std::string port = match[3];
    std::string addr = host + ":" + port;

    if (isSelf(peerId, host, port)) {
      mSelfInfo.mId = peerId;
      mSelfInfo.mAddress = addr;
    } else {
      Peer peer;
      peer.mId = peerId;
      peer.mAddress = addr;
      mPeers[peerId] = peer;
    }
  }

  assert(mSelfInfo.mId != kBadID);
  SPDLOG_INFO("cluster.size={}, self.id={}, self.address={}",
              mPeers.size() + 1, mSelfInfo.mId, mSelfInfo.mAddress);
}

void RaftCore::initStorage(const INIReader &iniReader) {
  auto storageType = iniReader.Get("raft.storage", "storage.type", "");

  if (storageType != "file") {
    /// in-memory log
    mLog = std::make_unique<storage::InMemoryLog>();
    return;
  }

  /// segment log
  auto storageDir = iniReader.Get("raft.storage", "storage.dir", "");
  auto dataSizeLimit = iniReader.GetInteger("raft.storage", "segment.data.size.limit", 0);
  auto metaSizeLimit = iniReader.GetInteger("raft.storage", "segment.meta.size.limit", 0);

  assert(!storageDir.empty() && dataSizeLimit > 0 && metaSizeLimit > 0);

  SPDLOG_INFO("Use SegmentLog, storage.dir={}, "
              "segment.data.size.limit={}, segment.meta.size.limit={}",
              storageDir, dataSizeLimit, metaSizeLimit);

  /// enable HMAC if needed
  auto crypto = std::make_shared<gringofts::CryptoUtil>();
  crypto->init(iniReader);

  mLog = std::make_unique<storage::SegmentLog>(storageDir, crypto, dataSizeLimit, metaSizeLimit);
}

void RaftCore::initService(const INIReader &iniReader) {
  auto tlsConfOpt = TlsUtil::parseTlsConf(iniReader, "raft.tls");

  /// init RaftServer
  mServer = std::make_unique<RaftServer>(mSelfInfo.mAddress, tlsConfOpt, &mAeRvQueue);

  /// init RaftClient
  for (const auto &p : mPeers) {
    auto &peer = p.second;
    mClients[peer.mId] = std::make_unique<RaftClient>(
        grpc::CreateChannel(peer.mAddress, TlsUtil::buildChannelCredentials(tlsConfOpt)),
        peer.mId,
        &mAeRvQueue);
  }

  /// sleep 10s, can we receive any AE_req from current Leader ?
  auto initialElectionTimeout = static_cast<uint64_t>(
      iniReader.GetInteger("raft.default", "initial.election.timeout", 10));

  mElectionTimePointInNano = TimeUtil::currentTimeInNanos()
      + initialElectionTimeout * 1000 * 1000 * 1000;

  auto isUnitTest = iniReader.GetBoolean("raft.default", "is.unit.test", false);

  /// skip setup of raft main loop for ut.
  if (isUnitTest) {
    SPDLOG_INFO("skip setup of raft main loop for ut.");
    return;
  }

  /// init raftMainLoop
  mRaftLoop = std::thread(&RaftCore::raftLoopMain, this);

  /// init StreamingService
  mStreamingService = std::make_unique<StreamingService>(iniReader, *this);
}

RaftCore::~RaftCore() {
  running = false;
  if (mRaftLoop.joinable()) {
    mRaftLoop.join();
  }
}

void RaftCore::raftLoopMain() {
  pthread_setname_np(pthread_self(), "RaftMainLoop");

  while (running) {
    /// message interaction
    appendEntries();
    requestVote();
    receiveMessage();

    /// following steps are done as single steps in part to:
    /// 1) minimize atomic regions,
    /// 2) support single-server cluster
    advanceCommitIndex();
    becomeLeader();
    electionTimeout();
    leadershipTimeout();
  }
}

void RaftCore::receiveMessage() {
  std::shared_ptr<RaftEventBase> event;

  if (!mAeRvQueue.empty()) {
    event = mAeRvQueue.dequeue();
  } else if (!mClientRequestsQueue.empty()) {
    event = mClientRequestsQueue.dequeue();
  } else {
    return;
  }

  TEST_POINT_WITH_TWO_ARGS(mTPProcessor,
      TPRegistry::RaftCore_receiveMessage_interceptIncoming, &mSelfInfo, event.get());

  /// AE_req
  if (event->mType == RaftEventBase::Type::AppendEntriesRequest) {
    auto ptr = dynamic_cast<AppendEntriesRequestEvent &>(*event).mPayload;
    (*ptr->mRequest.mutable_metrics()).set_request_event_dequeue_time(TimeUtil::currentTimeInNanos());

    handleAppendEntriesRequest(ptr->mRequest, &ptr->mResponse);

    (*ptr->mResponse.mutable_metrics()).set_response_send_time(TimeUtil::currentTimeInNanos());
    ptr->reply();
  }

  /// AE_resp
  if (event->mType == RaftEventBase::Type::AppendEntriesResponse) {
    auto ptr = std::move(dynamic_cast<AppendEntriesResponseEvent &>(*event).mPayload);
    (*ptr->mResponse.mutable_metrics()).set_response_event_dequeue_time(TimeUtil::currentTimeInNanos());

    /// turn on switch
    auto &peer = mPeers[ptr->mPeerId];
    auto hbIntervalInNano = RaftConstants::kHeartBeatIntervalInMillis * 1000 * 1000;
    peer.mNextRequestTimeInNano = std::max(peer.mLastRequestTimeInNano + hbIntervalInNano,
                                           TimeUtil::currentTimeInNanos());

    if (ptr->mStatus.ok()) {
      peer.mLastResponseTimeInNano = TimeUtil::currentTimeInNanos();
      handleAppendEntriesResponse(ptr->mResponse);
    } else {
      peer.mSuppressBulkData = true;
    }
  }

  /// RV_req
  if (event->mType == RaftEventBase::Type::RequestVoteRequest) {
    auto ptr = dynamic_cast<RequestVoteRequestEvent &>(*event).mPayload;
    handleRequestVoteRequest(ptr->mRequest, &ptr->mResponse);
    ptr->reply();
  }

  /// RV_resp
  if (event->mType == RaftEventBase::Type::RequestVoteResponse) {
    auto ptr = std::move(dynamic_cast<RequestVoteResponseEvent &>(*event).mPayload);
    auto &peer = mPeers[ptr->mPeerId];

    /// turn on switch
    auto hbIntervalInNano = RaftConstants::kHeartBeatIntervalInMillis * 1000 * 1000;
    peer.mNextRequestTimeInNano = std::max(peer.mLastRequestTimeInNano + hbIntervalInNano,
                                           TimeUtil::currentTimeInNanos());

    if (ptr->mStatus.ok()) {
      peer.mLastResponseTimeInNano = TimeUtil::currentTimeInNanos();
      handleRequestVoteResponse(ptr->mResponse);
    }
  }

  /// Cli_req
  if (event->mType == RaftEventBase::Type::ClientRequest) {
    ClientRequests clientRequests = std::move(dynamic_cast<ClientRequestsEvent &>(*event).mPayload);
    handleClientRequests(std::move(clientRequests));
  }
}

void RaftCore::appendEntries() {
  if (mRaftRole != RaftRole::Leader) {
    return;
  }

  for (auto &p : mPeers) {
    auto &peer = p.second;

    if (peer.mNextRequestTimeInNano > TimeUtil::currentTimeInNanos()) {
      continue;
    }

    /// build AE_req
    AppendEntries::Request request;
    (*request.mutable_metrics()).set_request_create_time(TimeUtil::currentTimeInNanos());

    auto currentTerm = mLog->getCurrentTerm();

    auto prevLogIndex = peer.mNextIndex - 1;
    auto prevLogTerm = termOfLogEntryAt(prevLogIndex);

    std::vector<LogEntry> entries;
    uint64_t batchSize = 0;

    if (!peer.mSuppressBulkData) {
      batchSize = mLog->getEntries(peer.mNextIndex,
                                   mMaxLenInBytes, mMaxBatchSize, &entries);
    }

    (*request.mutable_metrics()).set_term(currentTerm);
    (*request.mutable_metrics()).set_leader_id(mSelfInfo.mId);
    (*request.mutable_metrics()).set_entries_count(batchSize);
    (*request.mutable_metrics()).set_entries_reading_done_time(TimeUtil::currentTimeInNanos());

    auto commitIndex = std::min(mCommitIndex.load(), prevLogIndex + batchSize);

    request.set_term(currentTerm);
    request.set_leader_id(mSelfInfo.mId);
    request.set_prev_log_index(prevLogIndex);
    request.set_prev_log_term(prevLogTerm);
    request.set_commit_index(commitIndex);

    for (auto &entry : entries) {
      *request.add_entries() = std::move(entry);
    }

    (*request.mutable_metrics()).set_request_send_time(TimeUtil::currentTimeInNanos());

    /// send AE_req
    auto &client = *mClients[peer.mId];
    client.appendEntries(request);

    /// avoid printing trace for heartbeat.
    if (batchSize > 0) {
      SPDLOG_INFO("{} send AE_req to Follower {} for term {}, copy {} entries",
                  selfId(), peer.mId, currentTerm, batchSize);
    }

    /// turn off switch
    peer.mNextRequestTimeInNano = std::numeric_limits<uint64_t>::max();
    peer.mLastRequestTimeInNano = TimeUtil::currentTimeInNanos();
  }
}

void RaftCore::requestVote() {
  if (mRaftRole != RaftRole::Candidate) {
    return;
  }

  for (auto &p : mPeers) {
    auto &peer = p.second;

    if (peer.mRequestVoteDone
        || peer.mNextRequestTimeInNano > TimeUtil::currentTimeInNanos()) {
      continue;
    }

    /// build RV_req
    auto currentTerm = mLog->getCurrentTerm();
    auto lastLogIndex = mLog->getLastLogIndex();
    auto lastLogTerm = termOfLogEntryAt(lastLogIndex);

    RequestVote::Request request;

    request.set_term(currentTerm);
    request.set_candidate_id(mSelfInfo.mId);
    request.set_last_log_index(lastLogIndex);
    request.set_last_log_term(lastLogTerm);
    request.set_create_time_in_nano(TimeUtil::currentTimeInNanos());

    /// send RV_req
    auto &client = *mClients[peer.mId];
    client.requestVote(request);

    SPDLOG_INFO("{} send RV_req to Node {} for term {} "
                "with <lastLogIndex, lastLogTerm>=<{}, {}>",
                selfId(), peer.mId, currentTerm, lastLogIndex, lastLogTerm);

    /// turn off switch
    peer.mNextRequestTimeInNano = std::numeric_limits<uint64_t>::max();
    peer.mLastRequestTimeInNano = TimeUtil::currentTimeInNanos();
  }
}

void RaftCore::handleAppendEntriesRequest(const AppendEntries::Request &request,
                                          AppendEntries::Response *response) {
  auto currentTerm = mLog->getCurrentTerm();

  /// prepare AE_resp
  (*response->mutable_metrics()) = request.metrics();
  (*response->mutable_metrics()).set_follower_id(mSelfInfo.mId);
  (*response->mutable_metrics()).set_response_create_time(TimeUtil::currentTimeInNanos());

  response->set_term(currentTerm);
  response->set_success(false);
  response->set_id(mSelfInfo.mId);
  response->set_saved_term(request.term());
  response->set_saved_prev_log_index(request.prev_log_index());
  response->set_last_log_index(mLog->getLastLogIndex());
  response->set_match_index(0);

  if (request.term() < currentTerm) {
    SPDLOG_INFO("{} reject AE_req from Node {}, remoteTerm {} < currentTerm {}.",
                selfId(), request.leader_id(), request.term(), currentTerm);
    return;
  }

  /// adjust term of AE_resp
  response->set_term(request.term());

  if (request.term() > currentTerm
      || (request.term() == currentTerm && mRaftRole == RaftRole::Candidate)) {
    stepDown(request.term());
  }

  /// receive AE_req from current leader
  updateElectionTimePoint();

  if (!mLeaderId) {
    mLeaderId = request.leader_id();
    SPDLOG_INFO("All hail Leader {} for term {}", request.leader_id(), request.term());
  } else {
    assert(mLeaderId == request.leader_id());
  }

  bool logIsOk = request.prev_log_index() < mLog->getFirstLogIndex()
      || (request.prev_log_index() <= mLog->getLastLogIndex()
          && termOfLogEntryAt(request.prev_log_index()) == request.prev_log_term());
  if (!logIsOk) {
    SPDLOG_INFO("{} reject AE_req from Leader {}, log is not OK, <prevLogIndex, prevLogTerm>=<{}, {}>.",
                selfId(), request.leader_id(), request.prev_log_index(), request.prev_log_term());
    return;
  }

  std::vector<LogEntry> entries;
  entries.reserve(request.entries().size());

  auto index = request.prev_log_index();
  for (auto &entry : request.entries()) {
    /// no gap
    assert(entry.index() == ++index);

    if (entry.index() < mLog->getFirstLogIndex()) {
      /// We already snapshot and discarded this index, so presumably
      /// we've received a committed entry we once already had.
      continue;
    }

    if (entry.index() <= mLog->getLastLogIndex()) {
      if (entry.term() == termOfLogEntryAt(entry.index())) {
        continue;
      }

      /// should never truncate committed entries
      assert(entry.index() > mCommitIndex);

      /// truncate conflict entries
      auto lastIndexKept = entry.index() - 1;
      mLog->truncateSuffix(lastIndexKept);
    }

    entries.push_back(entry);
  }

  assert(mLog->appendEntries(entries));   /// mLog CAN handle empty entries.
  (*response->mutable_metrics()).set_entries_writing_done_time(TimeUtil::currentTimeInNanos());

  /// adjust AE_resp
  response->set_success(true);
  response->set_last_log_index(mLog->getLastLogIndex());
  response->set_match_index(request.prev_log_index() + request.entries().size());

  if (!request.entries().empty()) {
    SPDLOG_INFO("{} accept AE_req from Leader {} at <prevLogIndex, prevLogTerm>=<{}, {}>, "
                "receive {} entries, append {} entries",
                selfId(), request.leader_id(), request.prev_log_index(), request.prev_log_term(),
                request.entries().size(), entries.size());
  }

  if (mCommitIndex < request.commit_index()) {
    if (mCommitIndex != 0) {
      mCommitIndexCounter.increase(request.commit_index() - mCommitIndex);
    }

    mCommitIndex = request.commit_index();
    assert(mCommitIndex <= mLog->getLastLogIndex());

    printStatus("FollowerIncreaseCommitIndex");
  }

  /// reset election timer again to avoid punishing the leader for our own
  /// long disk writes
  updateElectionTimePoint();
}

void RaftCore::handleAppendEntriesResponse(const AppendEntries::Response &response) {
  auto currentTerm = mLog->getCurrentTerm();

  if (currentTerm != response.saved_term()) {
    /// we don't care about result of RPC
    return;
  }

  /// we were leader in this term before, we must still be leader in this term.
  assert(mRaftRole == RaftRole::Leader);

  if (response.term() > currentTerm) {
    SPDLOG_INFO("{} on term {} step down, "
                "due to receive AE_resp from Node {} with higher term {}",
                selfId(), currentTerm, response.id(), response.term());
    stepDown(response.term());
    return;
  }

  auto &peer = mPeers[response.id()];

  /// ignore duplicate AE_resp
  if (response.saved_prev_log_index() != peer.mNextIndex - 1) {
    SPDLOG_WARN("{} receive duplicate AE_resp from Follower {} "
                "with prevLogIndex={}, Peer with matchIndex={} and nextIndex={}.",
                selfId(), response.id(), response.saved_prev_log_index(),
                peer.mMatchIndex, peer.mNextIndex);
    return;
  }

  /// if a follower drops his log and starts from a specified firstIndex,
  /// make sure that his firstIndex is:
  /// 1) greater than or equal to leader's firstIndex,
  /// 2) greater than his matchIndex in leader.
  /// otherwise, his AE_resp will trigger assertion in leader.
  if (response.success()) {
    /// matchIndex increase monotonically
    assert(peer.mMatchIndex <= response.match_index());

    peer.mMatchIndex = response.match_index();
    peer.mNextIndex = peer.mMatchIndex + 1;
    peer.mSuppressBulkData = false;

    /// logging metrics
    printMetrics(response.metrics());
  } else {
    assert(peer.mMatchIndex <= response.last_log_index());

    /// there should be a gap between matchIndex and nextIndex
    assert(peer.mMatchIndex + 1 < peer.mNextIndex);

    if (peer.mNextIndex > response.last_log_index() + 1) {
      peer.mNextIndex = response.last_log_index() + 1;
    } else {
      auto prevNextIndex = peer.mNextIndex;
      auto binaryDecrStep = (peer.mNextIndex - peer.mMatchIndex) >> 1;
      peer.mNextIndex -= std::min(binaryDecrStep, mMaxDecrStep);
      SPDLOG_INFO("{} decrease nextIndex of Follower {} from {} to {}",
                  selfId(), response.id(), prevNextIndex, peer.mNextIndex);
    }
  }
}

void RaftCore::handleRequestVoteRequest(const RequestVote::Request &request,
                                        RequestVote::Response *response) {
  auto currentTerm = mLog->getCurrentTerm();

  /// prepare RV_resp
  response->set_term(currentTerm);
  response->set_vote_granted(false);
  response->set_create_time_in_nano(TimeUtil::currentTimeInNanos());
  response->set_id(mSelfInfo.mId);
  response->set_saved_term(request.term());

  if (request.term() < currentTerm) {
    SPDLOG_INFO("{} reject RV_req from Node {}, remoteTerm {} < currentTerm {}.",
                selfId(), request.candidate_id(), request.term(), currentTerm);
    return;
  }

  if (request.term() > currentTerm) {
    response->set_term(request.term());
    stepDown(request.term());
  }

  auto lastLogIndex = mLog->getLastLogIndex();
  auto lastLogTerm = termOfLogEntryAt(lastLogIndex);

  bool logIsOk = request.last_log_term() > lastLogTerm
      || (request.last_log_term() == lastLogTerm && request.last_log_index() >= lastLogIndex);

  auto voteFor = mLog->getVote();

  if (logIsOk && (voteFor == 0 || voteFor == request.candidate_id())) {
    mLog->setVoteFor(request.candidate_id());
    response->set_vote_granted(true);

    /// grant vote to candidate
    updateElectionTimePoint();

    SPDLOG_INFO("{} vote for Node {} in term {}.",
                selfId(), request.candidate_id(), request.term());
  } else {
    SPDLOG_INFO("{} reject RV_req from Node {}, logIsOk={}, voteFor={}",
                selfId(), request.candidate_id(), logIsOk, voteFor);
  }
}

void RaftCore::handleRequestVoteResponse(const RequestVote::Response &response) {
  auto currentTerm = mLog->getCurrentTerm();

  if (currentTerm != response.saved_term() || mRaftRole != RaftRole::Candidate) {
    SPDLOG_INFO("{} ignore stale RV_resp from Node {} for term {}",
                selfId(), response.id(), response.saved_term());
    return;
  }

  if (response.term() > currentTerm) {
    SPDLOG_INFO("{} receive RV_resp from Node {}, remoteTerm {} > currentTerm {}",
                selfId(), response.id(), response.term(), currentTerm);
    stepDown(response.term());
    return;
  }

  auto &peer = mPeers[response.id()];
  peer.mRequestVoteDone = true;

  if (response.vote_granted()) {
    peer.mHaveVote = true;
    SPDLOG_INFO("{} got vote from Node {} for term {}.",
                selfId(), response.id(), currentTerm);
  } else {
    SPDLOG_INFO("{} is reject by Node {} for term {}.",
                selfId(), response.id(), currentTerm);
  }
}

void RaftCore::handleClientRequests(ClientRequests clientRequests) {
  if (mRaftRole != RaftRole::Leader) {
    /// reject client request immediately
    for (auto &clientRequest : clientRequests) {
      auto &entry = clientRequest.mEntry;
      auto handle = clientRequest.mRequestHandle;

      if (handle != nullptr) {
        handle->fillResultAndReply(301, "NotLeaderAnyMore", mLeaderId);
      }
      SPDLOG_WARN("Not leader any more, discard entry "
                  "<index, term>=<{}, {}>.", entry.index(), entry.term());
    }
    return;
  }

  /// validate and filter entries that writes to WAL
  auto currentTerm = mLog->getCurrentTerm();
  auto lastLogIndex = mLog->getLastLogIndex();

  std::vector<LogEntry> entries;
  entries.reserve(clientRequests.size());

  for (auto &clientRequest : clientRequests) {
    auto &entry = clientRequest.mEntry;
    auto handle = clientRequest.mRequestHandle;

    if (entry.term() == currentTerm && entry.index() == lastLogIndex + 1) {
      mPendingClientRequests.emplace_back(std::make_pair(entry.index(), handle));
      entries.push_back(std::move(entry));
      ++lastLogIndex;
    } else {
      SPDLOG_WARN("{} discard entry, "
                  "expect <index, term>=<{}, {}>, received <index, term>=<{}, {}>",
                  selfId(), lastLogIndex + 1, currentTerm, entry.index(), entry.term());
    }
  }

  SPDLOG_INFO("{} on term {} append {} entry", selfId(), currentTerm, entries.size());
  mLog->appendEntries(entries);
}

void RaftCore::advanceCommitIndex() {
  if (mRaftRole != RaftRole::Leader) {
    return;
  }

  /// calculate the largest entry stored on a quorum of servers
  /// work for single-server cluster as well
  std::vector<uint64_t> indices;
  indices.push_back(mLog->getLastLogIndex());

  for (auto &p : mPeers) {
    auto &peer = p.second;
    indices.push_back(peer.mMatchIndex);
  }

  std::sort(indices.begin(), indices.end(),
            [](uint64_t x, uint64_t y) { return x > y; });

  auto majorityIndex = indices[indices.size() >> 1];

  /// commitIndex monotonically increase
  if (mCommitIndex >= majorityIndex) {
    return;
  }

  /// leader only commit log entry from its current term.
  if (termOfLogEntryAt(majorityIndex) != mLog->getCurrentTerm()) {
    return;
  }

  /// update counter
  if (mCommitIndex != 0) {
    mCommitIndexCounter.increase(majorityIndex - mCommitIndex);
  }

  mCommitIndex = majorityIndex;
  assert(mCommitIndex <= mLog->getLastLogIndex());

  printStatus("advanceCommitIndex");

  /// cleanup client request
  while (!mPendingClientRequests.empty()) {
    auto &p = mPendingClientRequests.front();

    auto index = p.first;
    auto handle = p.second;

    if (index > mCommitIndex) {
      break;
    }

    if (handle != nullptr) {
      handle->fillResultAndReply(200, "Success", mLeaderId);
    }

    mPendingClientRequests.pop_front();
  }
}

void RaftCore::becomeLeader() {
  if (mRaftRole != RaftRole::Candidate) {
    return;
  }

  /// candidate always vote for himself
  /// work for single-server cluster as well
  uint64_t voteNum = 1;

  for (const auto &p : mPeers) {
    auto &peer = p.second;
    if (peer.mHaveVote) {
      ++voteNum;
    }
  }

  auto quorumNum = getMajorityNumber(mPeers.size() + 1);
  if (voteNum < quorumNum) {
    return;
  }

  SPDLOG_INFO("{}, become Leader on term {}, voteNum={}, quorumNum={}.",
              selfId(), mLog->getCurrentTerm(), voteNum, quorumNum);

  mRaftRole = RaftRole::Leader;

  /// schedule a round of AE_req (heartbeat) immediately
  for (auto &p : mPeers) {
    auto &peer = p.second;

    peer.mNextIndex = mLog->getLastLogIndex() + 1;
    peer.mMatchIndex = 0;
    peer.mSuppressBulkData = true;

    /// turn on switch
    peer.mNextRequestTimeInNano = TimeUtil::currentTimeInNanos();
  }

  /// append noop
  LogEntry entry;

  entry.set_term(mLog->getCurrentTerm());
  entry.set_index(mLog->getLastLogIndex() + 1);
  entry.set_noop(true);
  entry.set_checksum(TimeUtil::currentTimeInNanos());

  assert(mLog->appendEntry(entry));

  printStatus("becomeLeader");

  /// notify monitor
  mLeadershipGauge.set(1);
}

void RaftCore::electionTimeout() {
  if (mRaftRole == RaftRole::Leader) {
    return;
  }

  TEST_POINT_WITH_TWO_ARGS(mTPProcessor, TPRegistry::RaftCore_electionTimeout_interceptTimeout,
      &mSelfInfo, &mElectionTimePointInNano);

  if (mElectionTimePointInNano > TimeUtil::currentTimeInNanos()) {
    return;
  }

  auto currentTerm = mLog->getCurrentTerm();
  SPDLOG_INFO("{} on term {} election timeout.", selfId(), currentTerm);

  /// become Candidate
  mRaftRole = RaftRole::Candidate;
  /// increment currentTerm
  mLog->setCurrentTerm(++currentTerm);
  /// vote for self
  mLog->setVoteFor(mSelfInfo.mId);
  /// clear leaderHint
  mLeaderId = 0;
  /// reset election timer
  updateElectionTimePoint();

  /// schedule a round of RV_req immediately
  for (auto &p : mPeers) {
    auto &peer = p.second;
    peer.mRequestVoteDone = false;
    peer.mHaveVote = false;

    /// turn on switch
    peer.mNextRequestTimeInNano = TimeUtil::currentTimeInNanos();
  }
}

void RaftCore::leadershipTimeout() {
  if (mRaftRole != RaftRole::Leader) {
    return;
  }

  auto nowInNano = TimeUtil::currentTimeInNanos();

  /// work for single-server cluster as well
  std::vector<uint64_t> timePoints;
  timePoints.push_back(nowInNano);

  for (const auto &p : mPeers) {
    auto &peer = p.second;
    timePoints.push_back(peer.mLastResponseTimeInNano);
  }

  /// sort by descending order
  std::sort(timePoints.begin(), timePoints.end(),
            [](uint64_t x, uint64_t y) { return x > y; });

  auto timeElapseInNano = nowInNano - timePoints[timePoints.size() >> 1];
  if (timeElapseInNano / 1000000.0 < RaftConstants::kMaxElectionTimeoutInMillis) {
    return;
  }

  auto currentTerm = mLog->getCurrentTerm();

  SPDLOG_INFO("{} on term {} stepDown due to lost authority, "
              "timeElapse={}ms, maxElectionTimeout={}ms",
              selfId(), currentTerm,
              timeElapseInNano / 1000000.0, RaftConstants::kMaxElectionTimeoutInMillis);

  stepDown(currentTerm + 1);
}

void RaftCore::stepDown(uint64_t newTerm) {
  auto currentTerm = mLog->getCurrentTerm();
  assert(currentTerm <= newTerm);

  /// Attention, must change role before update term.
  auto prevRole = mRaftRole;
  mRaftRole = RaftRole::Follower;

  if (currentTerm < newTerm) {
    mLog->setCurrentTerm(newTerm);
    mLog->setVoteFor(0);  /// 0 means vote for nobody
    mLeaderId = 0;
    SPDLOG_INFO("{} stepDown, increase term from {} to {}.",
                selfId(), currentTerm, newTerm);
  } else {
    assert(prevRole == RaftRole::Candidate);
    SPDLOG_INFO("{} on term {} stepDown.", selfId(), currentTerm);
  }

  /// step down from Leader
  if (prevRole == RaftRole::Leader) {
    /// cleanup client request
    while (!mPendingClientRequests.empty()) {
      auto &p = mPendingClientRequests.front();
      if (p.second != nullptr) {
        p.second->fillResultAndReply(301, "LeaderStepDown", mLeaderId);
      }
      mPendingClientRequests.pop_front();
    }

    /// notify monitor
    mLeadershipGauge.set(0);
    /// resume election timer
    updateElectionTimePoint();
  }
}

void RaftCore::printStatus(const std::string &reason) const {
  auto firstLogIndex = mLog->getFirstLogIndex();
  auto lastLogIndex = mLog->getLastLogIndex();

  SPDLOG_INFO("========== PRINT STATUS: {} ==========", reason);
  SPDLOG_INFO("firstLogIndex={}, lastLogIndex={}, commitIndex={}",
              firstLogIndex, lastLogIndex, mCommitIndex);

  if (mRaftRole == RaftRole::Leader) {
    for (auto &p : mPeers) {
      auto &peer = p.second;
      SPDLOG_INFO("Follower {}: nextIndex={}, matchIndex={}",
                  peer.mId, peer.mNextIndex, peer.mMatchIndex);
    }
  }

  auto startLogIndex = (lastLogIndex >= mMaxTailedEntryNum) ? lastLogIndex - mMaxTailedEntryNum + 1 : 1;
  startLogIndex = std::max(firstLogIndex, startLogIndex);

  for (auto i = startLogIndex; i <= lastLogIndex; ++i) {
    LogEntry entry;
    if (!mLog->getEntry(i, &entry)) {
      continue;
    }
    SPDLOG_INFO("index: {}, term: {}, noop: {}",
                entry.index(), entry.term(), entry.noop());
  }
}

void RaftCore::printMetrics(const AppendEntries::Metrics &metrics) {
  if (metrics.entries_count() == 0) {
    /// avoid printing trace for heartbeat
    return;
  }

  auto elapseInMillis = [](uint64_t beginInNano, uint64_t endInNano) -> double {
    return beginInNano > endInNano ? 0.0 : (endInNano - beginInNano) / 1000000.0;
  };

  SPDLOG_INFO("AE_metrics between Leader {} and Follower {}, entries.num={}, "
              "total.cost={}ms, "
              "read.entries.cost={}ms, "
              "request.build.cost={}ms, "
              "request.network.latency={}ms, "
              "request.queue.latency={}ms, "
              "write.entries.cost={}ms, "
              "response.network.latency={}ms, "
              "response.queue.latency={}ms.",
              metrics.leader_id(), metrics.follower_id(), metrics.entries_count(),
              elapseInMillis(metrics.request_create_time(), metrics.response_event_dequeue_time()),
              elapseInMillis(metrics.request_create_time(), metrics.entries_reading_done_time()),
              elapseInMillis(metrics.entries_reading_done_time(), metrics.request_send_time()),
              elapseInMillis(metrics.request_send_time(), metrics.request_event_enqueue_time()),
              elapseInMillis(metrics.request_event_enqueue_time(), metrics.request_event_dequeue_time()),
              elapseInMillis(metrics.response_create_time(), metrics.entries_writing_done_time()),
              elapseInMillis(metrics.response_send_time(), metrics.response_event_enqueue_time()),
              elapseInMillis(metrics.response_event_enqueue_time(), metrics.response_event_dequeue_time()));
}

/// for UT
RaftCore::RaftCore(const char *configPath, TestPointProcessor *processor): RaftCore(configPath, std::nullopt) {
  mTPProcessor = processor;
}

}  /// namespace v2
}  /// namespace raft
}  /// namespace gringofts

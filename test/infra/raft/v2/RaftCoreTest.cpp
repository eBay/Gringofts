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

#include <gtest/gtest.h>

#include "../../../../src/infra/raft/v2/RaftCore.h"
#include "../../../../src/infra/util/Util.h"

namespace gringofts::raft::v2 {

class RaftCoreTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Util::executeCmd("mkdir ../test/infra/raft/node_1");
    mRaftImpl = std::make_shared<RaftCore>("../test/infra/raft/config/raft_1.ini", std::nullopt);
  }

  void TearDown() override {
    mRaftImpl.reset();
    Util::executeCmd("rm -rf ../test/infra/raft/node_1");
  }

  std::shared_ptr<RaftCore> mRaftImpl;
};

TEST_F(RaftCoreTest, BasicTest) {
  /// init state of raft log
  ASSERT_EQ(mRaftImpl->getCommitIndex(), 0);
  ASSERT_EQ(mRaftImpl->getLastLogIndex(), 0);

  /// follower on term 0
  ASSERT_EQ(mRaftImpl->getCurrentTerm(), 0);
  ASSERT_EQ(mRaftImpl->getRaftRole(), RaftRole::Follower);

  /// election timeout
  mRaftImpl->mElectionTimePointInNano = 0;
  mRaftImpl->electionTimeout();

  /// candidate on term 1
  ASSERT_EQ(mRaftImpl->getCurrentTerm(), 1);
  ASSERT_EQ(mRaftImpl->getRaftRole(), RaftRole::Candidate);

  /// send RV_req
  mRaftImpl->requestVote();

  {
    /// fake RV_resp
    gringofts::raft::RequestVote::Response rvResp;
    rvResp.set_term(1);
    rvResp.set_vote_granted(true);
    rvResp.set_id(2);
    rvResp.set_saved_term(1);

    /// handle RV_resp
    mRaftImpl->handleRequestVoteResponse(rvResp);
    mRaftImpl->becomeLeader();
  }

  /// leader on term 1
  ASSERT_EQ(mRaftImpl->getCurrentTerm(), 1);
  ASSERT_EQ(mRaftImpl->getRaftRole(), RaftRole::Leader);

  /// send AE_req
  mRaftImpl->appendEntries();

  {
    /// fake AE_resp
    gringofts::raft::AppendEntries::Response aeResp;
    aeResp.set_term(1);
    aeResp.set_success(true);
    aeResp.set_id(2);
    aeResp.set_saved_term(1);
    aeResp.set_saved_prev_log_index(0);
    aeResp.set_last_log_index(1);
    aeResp.set_match_index(1);
    aeResp.mutable_metrics()->set_entries_count(1);

    /// handle AE_resp
    mRaftImpl->handleAppendEntriesResponse(aeResp);
    mRaftImpl->advanceCommitIndex();
  }

  /// fake Client Req
  ClientRequests clientRequests;

  for (uint64_t i = 2; i <= 10; ++i) {
    gringofts::raft::LogEntry entry;
    entry.set_index(i);
    entry.set_term(1);
    entry.set_noop(false);
    entry.set_payload("Hello, John Doe");

    clientRequests.emplace_back(ClientRequest{entry, nullptr});
  }

  /// handle Client req
  mRaftImpl->handleClientRequests(clientRequests);

  /// reject, due to invalid index
  mRaftImpl->handleClientRequests(clientRequests);

  /// step down, due to lost authority
  sleep(2);
  mRaftImpl->leadershipTimeout();

  /// reject, due to invalid role
  mRaftImpl->handleClientRequests(clientRequests);

  /// drain queue
  /// previous appendEntries() and requestVote() will generate events.
  for (uint64_t i = 0; i < 10; ++i) {
    mRaftImpl->receiveMessage();
  }

  {
    /// fake RV_req
    gringofts::raft::RequestVote::Request rvReq;
    gringofts::raft::RequestVote::Response rvResp;

    rvReq.set_term(4);
    rvReq.set_candidate_id(2);
    rvReq.set_last_log_index(10);
    rvReq.set_last_log_term(2);

    /// handle RV_req
    mRaftImpl->handleRequestVoteRequest(rvReq, &rvResp);
  }

  {
    /// fake AE_req
    gringofts::raft::AppendEntries::Request aeReq;
    gringofts::raft::AppendEntries::Response aeResp;

    aeReq.set_term(5);
    aeReq.set_leader_id(2);
    aeReq.set_prev_log_index(1);
    aeReq.set_prev_log_term(1);
    aeReq.set_commit_index(3);

    {
      gringofts::raft::LogEntry entry;
      entry.set_term(1);
      entry.set_index(2);
      entry.set_noop(true);

      *aeReq.add_entries() = std::move(entry);
    }
    {
      gringofts::raft::LogEntry entry;
      entry.set_term(2);
      entry.set_index(3);
      entry.set_noop(true);

      *aeReq.add_entries() = std::move(entry);
    }

    /// handle AE_req
    mRaftImpl->handleAppendEntriesRequest(aeReq, &aeResp);
  }

  {
    /// misc
    mRaftImpl->getLeaderHint();

    gringofts::raft::LogEntry entry;
    mRaftImpl->getEntry(1, &entry);

    std::vector<gringofts::raft::LogEntry> entries;
    mRaftImpl->getEntries(1, 1, &entries);

    mRaftImpl->truncatePrefix(1);
    mRaftImpl->enqueueClientRequests(clientRequests);
  }
}

}  /// namespace gringofts::raft::v2

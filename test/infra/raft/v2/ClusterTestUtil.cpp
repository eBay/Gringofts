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

#include "ClusterTestUtil.h"

#include <grpc++/grpc++.h>
#include <grpc++/security/credentials.h>


namespace gringofts {
namespace raft {
namespace v2 {

void ClusterTestUtil::setupAllServers(const std::vector<std::string> &configPaths) {
  killAllServers();
  for (auto &config : configPaths) {
    setupServer(config);
  }
}

void ClusterTestUtil::setupAllServers(
    const std::vector<std::string> &configPaths,
    const std::vector<SyncPoint> &points) {
  killAllServers();
  SyncPointProcessor::getInstance().setup(points);
  for (auto &config : configPaths) {
    setupServer(config);
  }
}

void ClusterTestUtil::killAllServers() {
  mRaftInsts.clear();
  mRaftInstClients.clear();
  SPDLOG_INFO("killing all servers");
  SyncPointProcessor::getInstance().tearDown();
}

void ClusterTestUtil::setupServer(const std::string &configPath) {
  SPDLOG_INFO("starting server using {}", configPath);
  std::shared_ptr<RaftCore> raftImpl(new RaftCore(configPath.c_str(), &SyncPointProcessor::getInstance()));
  const auto &member = raftImpl->mSelfInfo;
  assert(mRaftInsts.find(member) == mRaftInsts.end());
  mRaftInsts[member] = raftImpl;
  mRaftInstClients[member] = std::make_unique<RaftClient>(
      grpc::CreateChannel(member.toString(), grpc::InsecureChannelCredentials()),
      raftImpl->mSelfInfo.mId,
      &mAeRvQueue);
}

void ClusterTestUtil::killServer(const MemberInfo &member) {
  assert(mRaftInsts.find(member) != mRaftInsts.end());
  SPDLOG_INFO("killing server {}", member.toString());
  mRaftInsts.erase(member);
  SPDLOG_INFO("server {} is down", member.toString());
  mRaftInstClients.erase(member);
  SPDLOG_INFO("client {} is down", member.toString());
}

MemberInfo ClusterTestUtil::getMemberInfo(const MemberInfo &member) {
  assert(mRaftInsts.find(member) != mRaftInsts.end());
  return mRaftInsts[member]->mSelfInfo;
}

uint64_t ClusterTestUtil::getCommitIndex(const MemberInfo &member) {
  assert(mRaftInsts.find(member) != mRaftInsts.end());
  return mRaftInsts[member]->getCommitIndex();
}

uint64_t ClusterTestUtil::getLastLogIndex(const MemberInfo &member) {
  assert(mRaftInsts.find(member) != mRaftInsts.end());
  return mRaftInsts[member]->getLastLogIndex();
}

int ClusterTestUtil::sendClientRequest(
    std::vector<uint32_t> *errCodes,
    std::vector<uint64_t> *entryIndexes,
    const std::vector<std::string> &data,
    uint32_t retryLimit) {
  auto leader = waitAndGetLeader();
  assert(mRaftInsts.find(leader) != mRaftInsts.end());

  auto retry = retryLimit;
  while (retry-- > 0) {
    SPDLOG_INFO("sending client requests to leader {}, remaining retry: {}", leader.toString(), retry);
    auto targetCnt = data.size();
    errCodes->resize(targetCnt);
    entryIndexes->resize(targetCnt);
    SyncRequestHandle handler(targetCnt, errCodes);
    ClientRequests reqs;
    auto startIndex = mRaftInsts[leader]->getLastLogIndex() + 1;
    for (auto i = 0; i < data.size(); ++i) {
      gringofts::raft::LogEntry entry;
      entry.set_term(mRaftInsts[leader]->getCurrentTerm());
      (*entryIndexes)[i] = startIndex + i;
      entry.set_index(startIndex + i);
      entry.set_noop(false);
      entry.set_payload(data[i]);
      ClientRequest req = {entry, &handler};
      reqs.emplace_back(req);
    }
    mRaftInsts[leader]->enqueueClientRequests(reqs);
    uint32_t maxWaitTimes = 5;
    while (maxWaitTimes-- > 0 && handler.mFinishedCnt != targetCnt) {
      SPDLOG_INFO("waiting for response of client requests, cnt: {}, target: {}", handler.mFinishedCnt, targetCnt);
      sleep(1);
    }
    if (handler.mFinishedCnt == targetCnt) {
      return 200;
    }
  }

  return -1;
}

bool ClusterTestUtil::getEntry(const MemberInfo &member, uint64_t index, gringofts::raft::LogEntry *entry) {
  assert(mRaftInsts.find(member) != mRaftInsts.end());
  /// we have to use mLog since the RaftCore::getEntry has limit
  return mRaftInsts[member]->mLog->getEntry(index, entry);
}

MemberInfo ClusterTestUtil::waitAndGetLeader() {
  while (true) {
    for (auto &[member, raftImpl] : mRaftInsts) {
      if (raftImpl->mRaftRole == RaftRole::Leader) {
        SPDLOG_INFO("leader {} is elected, waiting for noop", member.toString());
        uint64_t leaderTerm = raftImpl->getCurrentTerm();
        uint32_t maxWaitNOOPTimes = 5;
        while (maxWaitNOOPTimes-- > 0) {
          uint64_t leaderLastIndex = raftImpl->getLastLogIndex();
          for (auto i = leaderLastIndex; i > 0; --i) {
            gringofts::raft::LogEntry entry;
            assert(raftImpl->mLog->getEntry(i, &entry));
            /// noop is committed for this term
            if (entry.term() == leaderTerm && entry.noop() && i <= raftImpl->getCommitIndex()) {
              return member;
            }
          }
          SPDLOG_INFO("noop is not committed for leader {}, wait: {}", member.toString(), maxWaitNOOPTimes);
          sleep(1);
        }
      }
    }
    SPDLOG_INFO("waiting for a leader to be elected");
    sleep(1);
  }
}

bool ClusterTestUtil::waitLogForAll(uint64_t index) {
  uint32_t retry = 10;
  while (retry-- > 0) {
    uint64_t cnt = 0;
    for (auto &[member, raftImpl] : mRaftInsts) {
      if (raftImpl->getLastLogIndex() >= index) {
        cnt += 1;
      }
    }
    if (cnt < mRaftInsts.size()) {
      SPDLOG_INFO("waiting for index {} to be appended, remaining retry: {}", index, retry);
      sleep(1);
    } else {
      return true;
    }
  }
  return false;
}

bool ClusterTestUtil::waitLogForServer(const MemberInfo &member, uint64_t index) {
  uint32_t retry = 10;
  while (retry-- > 0) {
    assert(mRaftInsts.find(member) != mRaftInsts.end());
    if (mRaftInsts[member]->getLastLogIndex() >= index) {
      return true;
    }
    SPDLOG_INFO("waiting for index {} of server {} to be appended, remaining retry: {}",
        index, member.toString(), retry);
    sleep(1);
  }
  return false;
}

bool ClusterTestUtil::waitLogCommitForAll(uint64_t index) {
  uint32_t retry = 10;
  while (retry-- > 0) {
    uint64_t cnt = 0;
    for (auto &[member, raftImpl] : mRaftInsts) {
      if (raftImpl->getCommitIndex() >= index) {
        cnt += 1;
      }
    }
    if (cnt < mRaftInsts.size()) {
      SPDLOG_INFO("waiting for index {} to be appended, remaining retry: {}", index, retry);
      sleep(1);
    } else {
      return true;
    }
  }
  return false;
}

bool ClusterTestUtil::waitLogCommitForServer(const MemberInfo &member, uint64_t index) {
  uint32_t retry = 20;
  while (retry-- > 0) {
    assert(mRaftInsts.find(member) != mRaftInsts.end());
    if (mRaftInsts[member]->getCommitIndex() >= index) {
      return true;
    }
    SPDLOG_INFO("waiting for index {} of server {} to be committed, remaining retry: {}",
        index, member.toString(), retry);
    sleep(1);
  }
  return false;
}

uint64_t ClusterTestUtil::countCommittedNodes(uint64_t index) {
  auto cnt = 0;
  for (auto &[member, raftImpl] : mRaftInsts) {
    if (raftImpl->getCommitIndex() >= index) {
      cnt += 1;
    }
  }
  return cnt;
}

}  /// namespace v2
}  /// namespace raft
}  /// namespace gringofts

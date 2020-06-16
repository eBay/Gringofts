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

#ifndef TEST_INFRA_RAFT_V2_CLUSTERTESTUTIL_H_
#define TEST_INFRA_RAFT_V2_CLUSTERTESTUTIL_H_

#include <map>

#include "../../../test_util/SyncPointProcessor.h"
#include "../../../../src/infra/raft/generated/raft.pb.h"
#include "../../../../src/infra/raft/RaftInterface.h"
#include "../../../../src/infra/raft/v2/RaftService.h"
#include "../../../../src/infra/raft/v2/RaftCore.h"

namespace gringofts {
namespace raft {
namespace v2 {

/// this handler will wait until the callback(fillResultAndReply) is invoked
class SyncRequestHandle : public RequestHandle {
 public:
    SyncRequestHandle(uint32_t targetCnt, std::vector<uint32_t> *errCodes):
      mTargetCnt(targetCnt), mErrCodes(errCodes) {
        assert(errCodes->size() == targetCnt);
      }
    void proceed() override {}
    void fillResultAndReply(
        uint32_t code,
        const std::string &message,
        std::optional<uint64_t> leaderId) override {
      assert(mFinishedCnt < mTargetCnt);
      (*mErrCodes)[mFinishedCnt++] = code;
    }
    std::atomic<uint32_t> mFinishedCnt = 0;
    uint32_t mTargetCnt = 0;
    std::vector<uint32_t> *mErrCodes;
};

class ClusterTestUtil {
 public:
    ClusterTestUtil() = default;

    /// disallow copy ctor and copy assignment
    ClusterTestUtil(const ClusterTestUtil &) = delete;
    ClusterTestUtil &operator=(const ClusterTestUtil &) = delete;

    /// disallow move ctor and move assignment
    ClusterTestUtil(ClusterTestUtil &&) = delete;
    ClusterTestUtil &operator=(ClusterTestUtil &&) = delete;

    void setupAllServers(const std::vector<std::string> &configPaths);
    void setupAllServers(const std::vector<std::string> &configPaths, const std::vector<SyncPoint> &points);
    void killAllServers();
    void setupServer(const std::string &configPath);
    void killServer(const MemberInfo &member);

    /// send request in sync
    int sendClientRequest(
        std::vector<uint32_t> *errCodes,
        std::vector<uint64_t> *entryIndexes,
        const std::vector<std::string> &data,
        uint32_t retryLimit = 1);

    MemberInfo getMemberInfo(const MemberInfo &member);
    uint64_t getCommitIndex(const MemberInfo &member);
    uint64_t getLastLogIndex(const MemberInfo &member);
    bool getEntry(const MemberInfo &member, uint64_t index, gringofts::raft::LogEntry *entry);

    MemberInfo waitAndGetLeader();
    bool waitLogForAll(uint64_t index);
    bool waitLogForServer(const MemberInfo &member, uint64_t index);
    bool waitLogCommitForAll(uint64_t index);
    bool waitLogCommitForServer(const MemberInfo &member, uint64_t index);
    uint64_t countCommittedNodes(uint64_t index);

    /// for sync point
    void enableAllSyncPoints() {
      SyncPointProcessor::getInstance().enableProcessing();
    }
    void disableAllSyncPoints() {
      SyncPointProcessor::getInstance().disableProcessing();
    }
    void resetSyncPoints(const std::vector<SyncPoint> &points) {
      SyncPointProcessor::getInstance().reset(points);
    }

 private:
    /// event queue should be destroyed after RaftClient
    EventQueue mAeRvQueue;

    std::map<MemberInfo, std::shared_ptr<RaftCore>> mRaftInsts;
    /// serve as client to send request to raft cluster
    std::map<MemberInfo, std::unique_ptr<RaftClient>> mRaftInstClients;
};

}  /// namespace v2
}  /// namespace raft
}  /// namespace gringofts

#endif  // TEST_INFRA_RAFT_V2_CLUSTERTESTUTIL_H_

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

#ifndef TEST_INFRA_RAFT_V2_RAFTMEMBERSHIPTEST_H_
#define TEST_INFRA_RAFT_V2_RAFTMEMBERSHIPTEST_H_

#include <set>
#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "ClusterTestUtil.h"
#include "../../../../src/infra/raft/v2/RaftCore.h"
#include "../../../../src/infra/util/FileUtil.h"
#include "../../../../src/infra/util/Util.h"

namespace gringofts {
namespace raft {
namespace v2 {

class RaftMembershipTest : public ::testing::Test {
 protected:
    SyncPointCallBack createCallBackToSelectLeader(const std::set<MemberInfo> &whitelistServers) {
      return [=](void *memberInfoArg, void *electionTimePointArg){
        auto info = static_cast<MemberInfo*>(memberInfoArg);
        auto electionTimePointInNano = static_cast<uint64_t*>(electionTimePointArg);
        if (whitelistServers.find(*info) == whitelistServers.end()) {
          /// sleep to avoid using too much CPU
          usleep(20 * 1000);
          *electionTimePointInNano = TimeUtil::currentTimeInNanos() +
            RaftConstants::kMaxElectionTimeoutInMillis * 1000 * 1000;
        }
      };
    }

    SyncPointCallBack createCallBackToReceiveMessages(const std::set<MemberInfo> &whitelistServers) {
      return [=](void *memberInfoArg, void *eventArg){
        auto info = static_cast<MemberInfo*>(memberInfoArg);
        auto event = static_cast<RaftEventBase*>(eventArg);
        if (whitelistServers.find(*info) == whitelistServers.end()) {
          event->mType = RaftEventBase::Type::Unknown;
        }
      };
    }

    ClusterTestUtil mClusterUtil;
};

}  /// namespace v2
}  /// namespace raft
}  /// namespace gringofts

#endif  // TEST_INFRA_RAFT_V2_RAFTMEMBERSHIPTEST_H_

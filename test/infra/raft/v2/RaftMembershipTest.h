/**
 * Copyright (c) 2020 eBay Software Foundation. All rights reserved.
 */

#ifndef TEST_INFRA_RAFT_V2_RAFTMEMBERSHIPTEST_H_
#define TEST_INFRA_RAFT_V2_RAFTMEMBERSHIPTEST_H_

#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>
#include <set>

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

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

#include "../../../../src/infra/raft/metrics/RaftMonitorAdaptor.h"
#include "../../../../src/infra/raft/v2/RaftCore.h"

namespace gringofts::test {

class RaftMonitorAdaptorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Util::executeCmd("mkdir ../test/infra/raft/node_1");
    mRaftImpl = std::make_shared<raft::v2::RaftCore>("../test/infra/raft/config/raft_1.ini", std::nullopt);
  }

  void TearDown() override {
    mRaftImpl.reset();
    Util::executeCmd("rm -rf ../test/infra/raft/node_1");
  }
  std::shared_ptr<raft::v2::RaftCore> mRaftImpl;
};

/// TODO(https://github.com/eBay/Gringofts/issues/10): Fix flaky test RaftMonitorAdaptorTest.raftAdaptorTest
TEST_F(RaftMonitorAdaptorTest, DISABLED_raftAdaptorTest) {
  RaftMonitorAdaptor raft_monitor_adaptor(mRaftImpl);
  auto tags = raft_monitor_adaptor.monitorTags();
  EXPECT_EQ(tags.size(), 4);
  EXPECT_STREQ(tags[0].name.c_str(), "current_role_v2");
  EXPECT_STREQ(tags[1].name.c_str(), "current_term_v2");
  EXPECT_STREQ(tags[2].name.c_str(), "committed_log_v2");
  EXPECT_STREQ(tags[3].name.c_str(), "last_index_v2");
}

}  /// namespace gringofts::test

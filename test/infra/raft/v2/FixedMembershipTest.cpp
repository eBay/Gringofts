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

#include "RaftMembershipTest.h"

namespace gringofts::raft::v2 {

class FixedMembershipTest : public RaftMembershipTest {
 protected:
    void SetUp() override {
      Util::executeCmd("rm -rf ../test/infra/raft/cluster_node1");
      Util::executeCmd("rm -rf ../test/infra/raft/cluster_node2");
      Util::executeCmd("rm -rf ../test/infra/raft/cluster_node3");
      Util::executeCmd("mkdir ../test/infra/raft/cluster_node1");
      Util::executeCmd("mkdir ../test/infra/raft/cluster_node2");
      Util::executeCmd("mkdir ../test/infra/raft/cluster_node3");
    }

    void TearDown() override {
      Util::executeCmd("rm -rf ../test/infra/raft/cluster_node1");
      Util::executeCmd("rm -rf ../test/infra/raft/cluster_node2");
      Util::executeCmd("rm -rf ../test/infra/raft/cluster_node3");
    }
};

TEST_F(FixedMembershipTest, NormalTest) {
  const MemberInfo server1{1, "0.0.0.0:11111"};
  const MemberInfo server2{2, "0.0.0.0:22222"};
  const MemberInfo server3{3, "0.0.0.0:33333"};

  /// fixed_member config
  const std::string server1Config = "../test/infra/raft/cluster_config/fixed_member_raft_node1.ini";
  const std::string server2Config = "../test/infra/raft/cluster_config/fixed_member_raft_node2.ini";
  const std::string server3Config = "../test/infra/raft/cluster_config/fixed_member_raft_node3.ini";
  SPDLOG_INFO("initializing raft servers");
  mClusterUtil.setupAllServers({server1Config, server2Config, server3Config});

  const std::vector<std::string> data = {
    "testdata1",
    "testdata2",
    "testdata3",
    "testdata4",
    "testdata5",
  };
  std::vector<uint32_t> errCodes;
  std::vector<uint64_t> indexs;
  ASSERT_EQ(mClusterUtil.sendClientRequest(&errCodes, &indexs, data), 200);
  uint64_t maxIndex = 0;
  /// verify leader data consistency
  auto leader = mClusterUtil.waitAndGetLeader();
  for (auto i = 0; i < data.size(); ++i) {
    ASSERT_EQ(errCodes[i], 200);
    gringofts::raft::LogEntry dataEntry;
    ASSERT_TRUE(mClusterUtil.getEntry(leader, indexs[i], &dataEntry));
    ASSERT_EQ(dataEntry.payload(), data[i]);
    maxIndex = maxIndex < indexs[i] ? indexs[i] : maxIndex;
  }
  auto majority = 3/2 + 1;
  auto count = mClusterUtil.countCommittedNodes(maxIndex);
  ASSERT_TRUE(count >= majority);
  /// wait all to commit so that we can verify
  ASSERT_TRUE(mClusterUtil.waitLogCommitForAll(maxIndex));
  /// verify all servers data consistency
  for (auto i = 0; i < data.size(); ++i) {
    gringofts::raft::LogEntry dataEntry;
    for (auto &s : {server1, server2, server3}) {
      ASSERT_TRUE(mClusterUtil.getEntry(s, indexs[i], &dataEntry));
      ASSERT_EQ(dataEntry.payload(), data[i]);
    }
  }
  SPDLOG_INFO("destroying all server");
  mClusterUtil.killAllServers();
}

}  /// namespace gringofts::raft::v2

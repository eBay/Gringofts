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

  const MemberInfo mServer1{1, "0.0.0.0:11111"};
  const MemberInfo mServer2{2, "0.0.0.0:22222"};
  const MemberInfo mServer3{3, "0.0.0.0:33333"};

  /// fixed_member config
  const std::string mServer1Config = "../test/infra/raft/cluster_config/mock_app_node1.ini";
  const std::string mServer2Config = "../test/infra/raft/cluster_config/mock_app_node2.ini";
  const std::string mServer3Config = "../test/infra/raft/cluster_config/mock_app_node3.ini";
  const std::vector<std::string> mData = {
    "testdata1",
    "testdata2",
    "testdata3",
    "testdata4",
    "testdata5",
  };
};

TEST_F(FixedMembershipTest, ClusterInfoTest) {
  mClusterUtil.setupAllServers({mServer1Config, mServer2Config, mServer3Config});
  auto allMembers = mClusterUtil.getAllMemberInfo();
  ASSERT_EQ(allMembers[0].mId, 1);
  ASSERT_EQ(allMembers[0].mAddress, "0.0.0.0:11111");
  ASSERT_EQ(allMembers[1].mId, 2);
  ASSERT_EQ(allMembers[1].mAddress, "0.0.0.0:22222");
  ASSERT_EQ(allMembers[2].mId, 3);
  ASSERT_EQ(allMembers[2].mAddress, "0.0.0.0:33333");
  mClusterUtil.killAllServers();
}

TEST_F(FixedMembershipTest, NormalTest) {
  mClusterUtil.enableAllSyncPoints();
  /// select server1 because it has lowest version, which is compatible with other servers
  SyncPointCallBack electServer1CB = createCallBackToSelectLeader({mServer1});
  std::vector<SyncPoint> point {
    {TPRegistry::RaftCore_electionTimeout_interceptTimeout, electServer1CB, {}, SyncPointType::Ignore}
  };
  SPDLOG_INFO("initializing raft servers");
  mClusterUtil.setupAllServers({mServer1Config, mServer2Config, mServer3Config}, point);

  std::vector<uint32_t> errCodes;
  std::vector<uint64_t> indexs;
  ASSERT_EQ(mClusterUtil.sendClientRequest(&errCodes, &indexs, mData), 200);
  uint64_t maxIndex = 0;
  /// verify leader data consistency
  auto leader = mClusterUtil.waitAndGetLeader();
  for (auto i = 0; i < mData.size(); ++i) {
    ASSERT_EQ(errCodes[i], 200);
    gringofts::raft::LogEntry dataEntry;
    ASSERT_TRUE(mClusterUtil.getDecryptedEntry(leader, indexs[i], &dataEntry));
    ASSERT_EQ(dataEntry.payload(), mData[i]);
    maxIndex = maxIndex < indexs[i] ? indexs[i] : maxIndex;
  }
  auto majority = 3/2 + 1;
  auto count = mClusterUtil.countCommittedNodes(maxIndex);
  ASSERT_TRUE(count >= majority);
  /// wait all to commit so that we can verify
  ASSERT_TRUE(mClusterUtil.waitLogCommitForAll(maxIndex));
  /// verify all servers data consistency
  for (auto i = 0; i < mData.size(); ++i) {
    gringofts::raft::LogEntry dataEntry;
    for (auto &s : {mServer1, mServer2, mServer3}) {
      ASSERT_TRUE(mClusterUtil.getDecryptedEntry(s, indexs[i], &dataEntry));
      ASSERT_EQ(dataEntry.payload(), mData[i]);
    }
  }
  SPDLOG_INFO("destroying all server");
  mClusterUtil.killAllServers();
  mClusterUtil.disableAllSyncPoints();
}

/// in this set up, server1 has 1 version, server2 has 2 versions, server3 has 3 versions
/// if we elect server1, then the cluster works fine
/// if we elect server2, then server1 will crash
/// if we elect server3, then server1/2 will crash
/// TODO qiawu will try to fix this test
TEST_F(FixedMembershipTest, DISABLED_SecretKeyVersionMissMatchTest) {
  testing::GTEST_FLAG(death_test_style) = "threadsafe";
  auto runWithChosenLeader = [this](const std::set<MemberInfo> &allowedLeaders) {
    mClusterUtil.enableAllSyncPoints();
    SyncPointCallBack electServerCB = createCallBackToSelectLeader(allowedLeaders);
    std::vector<SyncPoint> ps {
      {TPRegistry::RaftCore_electionTimeout_interceptTimeout, electServerCB, {}, SyncPointType::Ignore}
    };

    SPDLOG_INFO("initializing raft servers");
    mClusterUtil.setupAllServers({mServer1Config, mServer2Config, mServer3Config}, ps);

    std::vector<uint32_t> errCodes;
    std::vector<uint64_t> indexs;
    /// use assert not ASSERT so that it could be captured by EXPECT_DEATH
    assert(mClusterUtil.sendClientRequest(&errCodes, &indexs, mData) == 200);
    /// verify all servers data consistency
    for (auto i = 0; i < mData.size(); ++i) {
      assert(errCodes[i] == 200);
      gringofts::raft::LogEntry dataEntry;
      for (auto &s : {mServer1, mServer2, mServer3}) {
        assert(mClusterUtil.waitLogForServer(s, indexs[i]));
        assert(mClusterUtil.getDecryptedEntry(s, indexs[i], &dataEntry));
        assert(dataEntry.payload() == mData[i]);
      }
    }
    SPDLOG_INFO("destroying all server");
    mClusterUtil.killAllServers();
    mClusterUtil.disableAllSyncPoints();
    exit(0);
  };
  SPDLOG_INFO("choosing server1, which has out-of-date key version, will work fine");
  EXPECT_EXIT(runWithChosenLeader({mServer1}), ::testing::ExitedWithCode(0), "");
  SPDLOG_INFO("choosing server2, which has more up-to-date key version then server1, will crash server1");
  EXPECT_DEATH(runWithChosenLeader({mServer2}), "");
  SPDLOG_INFO("choosing server3, which has more up-to-date key version, will crash server2/server3");
  EXPECT_DEATH(runWithChosenLeader({mServer3}), "");
}

/// in this set up, server1 has 1 version, server2 has 2 versions, server3 has 3 versions
/// in this test, we do following
/// 1. elect server1, the cluster should work fine with version 1
/// 2. bring down server1, elect server2, the cluster should work fine with version 2
/// 3. recover server1 with three key versions,
///    the cluster should work well with version 2 because server2 is still the leader
/// 4. bring down server2, and elect server1, the cluster should use latest key version 3
TEST_F(FixedMembershipTest, SecretKeyVersionUpgradeTest) {
  auto sendReqAndVerifySecKeyVersion = [this](const std::vector<MemberInfo> &serversToVerify, SecKeyVersion expected) {
    std::vector<uint32_t> errCodes;
    std::vector<uint64_t> indexs;
    EXPECT_EQ(mClusterUtil.sendClientRequest(&errCodes, &indexs, mData), 200);
    /// verify all servers data consistency
    for (auto i = 0; i < mData.size(); ++i) {
      EXPECT_EQ(errCodes[i], 200);
      gringofts::raft::LogEntry dataEntry;
      for (auto &s : serversToVerify) {
        EXPECT_TRUE(mClusterUtil.waitLogForServer(s, indexs[i]));
        EXPECT_TRUE(mClusterUtil.getDecryptedEntry(s, indexs[i], &dataEntry));
        EXPECT_EQ(dataEntry.version().secret_key_version(), expected);
        assert(dataEntry.payload() == mData[i]);
      }
    }
  };

  mClusterUtil.enableAllSyncPoints();
  /// step1
  SyncPointCallBack electServer1CB = createCallBackToSelectLeader({mServer1});
  std::vector<SyncPoint> psInStep1 {
    {TPRegistry::RaftCore_electionTimeout_interceptTimeout, electServer1CB, {}, SyncPointType::Ignore}
  };

  SPDLOG_INFO("initializing raft servers");
  mClusterUtil.setupAllServers({mServer1Config, mServer2Config, mServer3Config}, psInStep1);
  {
    SecKeyVersion expectedVersion = 1;
    sendReqAndVerifySecKeyVersion({mServer1, mServer2, mServer3}, expectedVersion);
  }
  /// step2
  mClusterUtil.killServer(mServer1);
  SyncPointCallBack electServer2CB = createCallBackToSelectLeader({mServer2});
  std::vector<SyncPoint> psInStep2 {
    {TPRegistry::RaftCore_electionTimeout_interceptTimeout, electServer2CB, {}, SyncPointType::Ignore}
  };
  mClusterUtil.resetSyncPoints(psInStep2);
  {
    SecKeyVersion expectedVersion = 2;
    sendReqAndVerifySecKeyVersion({mServer2, mServer3}, expectedVersion);
  }
  /// step3
  mClusterUtil.setupServer("../test/infra/raft/cluster_config/mock_app_node1.upgrade_key_version.ini");
  {
    SecKeyVersion expectedVersion = 2;
    sendReqAndVerifySecKeyVersion({mServer1, mServer2, mServer3}, expectedVersion);
  }

  /// step4
  mClusterUtil.killServer(mServer2);
  std::vector<SyncPoint> psInStep4 {
    {TPRegistry::RaftCore_electionTimeout_interceptTimeout, electServer1CB, {}, SyncPointType::Ignore}
  };
  mClusterUtil.resetSyncPoints(psInStep4);
  {
    SecKeyVersion expectedVersion = 3;
    sendReqAndVerifySecKeyVersion({mServer1, mServer3}, expectedVersion);
  }

  SPDLOG_INFO("destroying all server");
  mClusterUtil.killAllServers();
  mClusterUtil.disableAllSyncPoints();
}
}  /// namespace gringofts::raft::v2

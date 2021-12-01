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
#include <spdlog/spdlog.h>
#include <absl/strings/str_format.h>
#include "../../../src/infra/util/ClusterInfo.h"
#include "../../../src/infra/util/Util.h"

using gringofts::ClusterInfo;

TEST(ClusterInfoTest, routeInfoTest) {
  EXPECT_FALSE(ClusterInfo::checkHasRoute("aa", 0, 0));
  EXPECT_FALSE(ClusterInfo::checkHasRoute("{\"epoch\":\"a1\"}", 0, 0));
  EXPECT_TRUE(ClusterInfo::checkHasRoute(
      "{\"epoch\":1,\"routeInfos\":[{\"clusterId\":0,\"routeEntries\":[{\"groups\":[0,1,2,3,4,5,6,7],\"groupTotal\":8}]},{\"clusterId\":1,\"routeEntries\":[{\"groups\":[0,1,2,3,4,5,6,7],\"groupTotal\":8}]}]}",  // NOLINT
      1,
      1));
  // true but warning since epoch is less than 2
  EXPECT_TRUE(ClusterInfo::checkHasRoute(
      "{\"epoch\":1,\"routeInfos\":[{\"clusterId\":0,\"routeEntries\":[{\"groups\":[0,1,2,3,4,5,6,7],\"groupTotal\":8}]},{\"clusterId\":1,\"routeEntries\":[{\"groups\":[0,1,2,3,4,5,6,7],\"groupTotal\":8}]}]}",  // NOLINT
      1,
      2));
  // false since can't find cluster 2
  EXPECT_FALSE(ClusterInfo::checkHasRoute(
      "{\"epoch\":1,\"routeInfos\":[{\"clusterId\":0,\"routeEntries\":[{\"groups\":[0,1,2,3,4,5,6,7],\"groupTotal\":8}]},{\"clusterId\":1,\"routeEntries\":[{\"groups\":[0,1,2,3,4,5,6,7],\"groupTotal\":8}]}]}",  // NOLINT
      2,
      0));
}

class ExternalClientMock : public gringofts::kv::Client {
 public:
  using Status = gringofts::kv::Status;
  using KVPair = gringofts::kv::KVPair;
  ExternalClientMock(std::vector<std::string> endPoints, std::optional<gringofts::TlsConf> tlsOpt) {
    EXPECT_EQ(endPoints.size(), 2);
    EXPECT_STREQ(endPoints[0].c_str(), "service.node.1:2379");
    EXPECT_STREQ(endPoints[1].c_str(), "service.node.2:2379");
    EXPECT_FALSE(tlsOpt);
    SPDLOG_INFO("init mocked external client");
  }
  Status getPrefix(const std::string &prefix, std::vector<KVPair> *out) const override {
    throw std::runtime_error("not implement");
  };
  Status getValue(const std::string &prefix, std::string *out) const override {
    *out = mKv[prefix];
    return Status::OK;
  }
  static std::unordered_map<std::string, std::string> mKv;
};

std::unordered_map<std::string, std::string> ExternalClientMock::mKv;

TEST(ClusterInfoTest, resolveAllClustersUsingExternal) {
  using gringofts::kv::ClientFactory_t;
  using gringofts::Signal;
  using gringofts::RouteSignal;
  {
    ExternalClientMock::mKv["cluster.conf"] =
        absl::StrFormat("1#1@node11.ebay.com,2@%s,3@node13.ebay.com", gringofts::Util::getHostname());
    auto[clusterId, nodeId, allClusterInfo] = ClusterInfo::resolveAllClusters(
        INIReader("../test/infra/util/config/cluster_external.ini"),
        std::make_unique<ClientFactory_t<ExternalClientMock>>());
    EXPECT_EQ(clusterId, 1);
    EXPECT_EQ(nodeId, 2);
    auto routeSignal = std::make_shared<gringofts::RouteSignal>(1, 1);
    Signal::hub << routeSignal;
    EXPECT_FALSE(routeSignal->getFuture().get());
  }
  {
    ExternalClientMock::mKv["cluster.conf"] =
        absl::StrFormat("1#1@node11.ebay.com,2@%s,3@node13.ebay.com", gringofts::Util::getHostname());
    ExternalClientMock::mKv["cluster.route"] =
        "{\"epoch\":1,\"routeInfos\":[{\"clusterId\":0,\"routeEntries\":[{\"groups\":[0,1,2,3,4,5,6,7],\"groupTotal\":8}]},{\"clusterId\":1,\"routeEntries\":[{\"groups\":[0,1,2,3,4,5,6,7],\"groupTotal\":8}]}]}";  // NOLINT
    auto[clusterId, nodeId, allClusterInfo] = ClusterInfo::resolveAllClusters(
        INIReader("../test/infra/util/config/cluster_external.ini"),
        std::make_unique<ClientFactory_t<ExternalClientMock>>());
    EXPECT_EQ(clusterId, 1);
    EXPECT_EQ(nodeId, 2);
    auto routeSignal = std::make_shared<gringofts::RouteSignal>(1, 1);
    Signal::hub << routeSignal;
    EXPECT_TRUE(routeSignal->getFuture().get());
  }
}

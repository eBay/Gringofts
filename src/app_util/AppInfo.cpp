/************************************************************************
Copyright 2019-2021 eBay Inc.
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

#include <absl/strings/str_split.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <spdlog/spdlog.h>

#include "AppInfo.h"

namespace gringofts::app {

bool AppClusterParser::checkHasRoute(const std::string &routeStr, uint64_t clusterId, uint64_t epoch) {
  using boost::property_tree::ptree;
  using boost::property_tree::read_json;
  using boost::property_tree::write_json;
  using boost::property_tree::json_parser_error;
  std::stringstream ss(routeStr);
  ptree globalRoute;
  try {
    read_json(ss, globalRoute);
    auto routeEpoch = std::stoi(globalRoute.get_child("epoch").data());
    if (routeEpoch < epoch) {
      SPDLOG_WARN("global epoch {} is less than local epoch {}", routeEpoch, epoch);
    }
    auto infos = globalRoute.get_child("routeInfos");
    for (auto &[k, v] : infos) {
      auto clusterNode = v.get_child("clusterId");
      auto id = std::stoi(clusterNode.data());
      if (clusterId == id) {
        std::stringstream sout;
        write_json(sout, v);
        SPDLOG_INFO("find route for cluster {} : {}", clusterId, sout.str());
        return true;
      }
    }
    return false;
  } catch (const json_parser_error &err) {
    SPDLOG_ERROR("error when parse json {} for {}", routeStr, err.message());
    return false;
  } catch (const std::exception &err) {
    SPDLOG_ERROR("error when parse json {} for {}", routeStr, err.what());
    return false;
  }
}

std::tuple<NodeId, ClusterId, ClusterParser::ClusterMap> AppClusterParser::parse(const INIReader &iniReader) {
  std::string storeType = iniReader.Get("cluster", "persistence.type", "UNKNOWN");
  assert(storeType == "raft");
  bool externalEnabled = iniReader.GetBoolean("raft.external", "enable", false);
  if (!externalEnabled) {
    /// load from local config, the cluster id and node id must be specified
    auto clusterConf = iniReader.Get("cluster", "cluster.conf", "");
    auto allClusterInfo = parseToClusterInfo(clusterConf);
    auto myClusterId = iniReader.GetInteger("cluster", "self.clusterId", 0);
    auto myNodeId = iniReader.GetInteger("cluster", "self.nodeId", 0);
    bool hasMe = false;
    for (auto &[clusterId, info] : allClusterInfo) {
      if (myClusterId == clusterId) {
        for (auto &[nodeId, node] : info.getAllNodes()) {
          if (nodeId == myNodeId) {
            hasMe = true;
            break;
          }
        }
      }
    }
    assert(hasMe);

    Signal::hub.handle<RouteSignal>([](const Signal &s) {
      const auto &signal = dynamic_cast<const RouteSignal &>(s);
      SPDLOG_WARN("for non-external controlled cluster direct start raft");
      signal.passValue(true);
    });

    SPDLOG_INFO("read raft cluster conf from local, "
                "cluster.conf={}, self.clusterId={}, self.nodeId={}",
                clusterConf, myClusterId, myNodeId);
    return {myClusterId, myNodeId, allClusterInfo};
  } else {
    // if enable external kv store for cluster info, it must have kv factory
    assert(mKvFactory);
    std::string externalConfigFile = iniReader.Get("raft.external", "config.file", "");
    std::string clusterConfKey = iniReader.Get("raft.external", "cluster.conf.key", "");
    std::string clusterRouteKey = iniReader.Get("raft.external", "cluster.route.key", "");
    assert(!externalConfigFile.empty());
    assert(!clusterConfKey.empty());

    /// init external client
    auto client = mKvFactory->produce(INIReader(externalConfigFile));
    /// read raft cluster conf from external
    std::string clusterConf;
    auto r = client->getValue(clusterConfKey, &clusterConf);
    assert(!clusterConfKey.empty());
    auto allClusterInfo = parseToClusterInfo(clusterConf);
    /// N.B.: when using external, the assumption is two nodes will never run on the same host,
    ///       otherwise below logic will break.
    auto myHostname = Util::getHostname();
    std::optional<ClusterId> myClusterId = std::nullopt;
    std::optional<NodeId> myNodeId = std::nullopt;
    for (auto &[clusterId, info] : allClusterInfo) {
      for (auto &[nodeId, node] : info.getAllNodes()) {
        if (myHostname == node->hostName()) {
          myClusterId = clusterId;
          myNodeId = nodeId;
          break;
        }
      }
    }
    assert(myClusterId);
    assert(myNodeId);

    SPDLOG_INFO("raft cluster conf passed from external, "
                "cluster.conf={}, hostname={}", clusterConf, myHostname);
    auto clusterId = *myClusterId;

    Signal::hub.handle<RouteSignal>([client, clusterRouteKey, clusterId](const Signal &s) {
      const auto &signal = dynamic_cast<const RouteSignal &>(s);
      std::string val;
      SPDLOG_INFO("receive signal for query route, cluster {}, epoch {}", clusterId, signal.mEpoch);
      assert(clusterId == signal.mClusterId);
      assert(!clusterRouteKey.empty());
      client->getValue(clusterRouteKey, &val);
      signal.passValue(checkHasRoute(val, clusterId, signal.mEpoch));
    });

    return {*myClusterId, *myNodeId, allClusterInfo};
  }
}

std::unordered_map<ClusterId, Cluster> AppClusterParser::parseToClusterInfo(const std::string &infoStr) const {
  std::unordered_map<ClusterId, Cluster> result;
  std::vector<std::string> clusters = absl::StrSplit(infoStr, ";");
  for (auto &c : clusters) {
    Cluster info;
    std::pair<std::string, std::string> clusterIdWithNodes = absl::StrSplit(c, "#");
    info.setClusterId(std::stoi(clusterIdWithNodes.first));
    std::vector<std::string> nodes = absl::StrSplit(clusterIdWithNodes.second, ",");
    for (auto &n : nodes) {
      std::pair<std::string, std::string> hostWithPort = absl::StrSplit(n, ":");
      std::pair<std::string, std::string> idWithHost = absl::StrSplit(hostWithPort.first, "@");
      auto nodeId = std::stoi(idWithHost.first);
      auto hostname = idWithHost.second;
      std::shared_ptr<Node> node;
      if (hostWithPort.second.empty()) {
        SPDLOG_INFO("{} no specific port, using default one", hostWithPort.second);
        node = std::make_shared<AppNode>(nodeId, hostname);
      } else {
        std::vector<std::string> ports = absl::StrSplit(hostWithPort.second, "|");
        assert(ports.size() == 6);
        auto portForRaft = std::stoi(ports[0]);
        auto portForGateway = std::stoi(ports[1]);
        auto portForDumper = std::stoi(ports[2]);
        auto portForStream = std::stoi(ports[3]);
        auto portForNetAdmin = std::stoi(ports[4]);
        auto portForScale = std::stoi(ports[5]);
        node = std::make_shared<AppNode>(nodeId, hostname, portForRaft, portForStream,
                                         portForGateway, portForDumper, portForNetAdmin, portForScale);
      }
      info.addNode(node);
    }
    result[info.id()] = info;
  }
  return result;
}

void AppInfo::init(const INIReader &reader, std::unique_ptr<ClusterParser> parser) {
  auto &appInfo = getInstance();

  auto &initialized = appInfo.initialized;
  bool expected = false;

  if (!initialized.compare_exchange_strong(expected, true)) {
    SPDLOG_WARN("appInfo has already been initialized, ignore.");
    return;
  }

  auto[myClusterId, myNodeId, allClusterInfo] = parser->parse(reader);
  appInfo.mMyClusterId = myClusterId;
  appInfo.mMyNodeId = myNodeId;
  appInfo.mAllClusterInfo = allClusterInfo;

  appInfo.setSubsystemId(reader.GetInteger("app", "subsystem.id", 0));
  appInfo.setGroupId(appInfo.mMyClusterId);
  appInfo.setGroupVersion(reader.GetInteger("app", "group.version", 0));
  appInfo.enableStressTest(reader.GetBoolean("app", "stress.test.enabled", false));
  appInfo.setAppVersion(reader.Get("app", "version", "v2"));

  SPDLOG_INFO("Global settings: subsystem.id={}, "
              "group.id={}, "
              "group.version={}, "
              "stress.test.enabled={}, "
              "app.version={}"
              "app.clusterid={}"
              "app.nodeid={}",
              appInfo.mSubsystemId,
              appInfo.mMyClusterId,
              appInfo.mGroupVersion,
              appInfo.mStressTestEnabled,
              appInfo.mAppVersion,
              appInfo.mMyClusterId,
              appInfo.mMyNodeId);
}

}  /// namespace gringofts::app

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

#include "AppInfo.h"

#include <spdlog/spdlog.h>
#include <absl/strings/str_format.h>
#include <absl/strings/str_split.h>



namespace gringofts::app {

AppInfo::AppInfo(const INIReader &reader) {
  assert(parseAllClusters(reader));
  mSubsystemId = reader.GetInteger("app", "subsystem.id", 0);
  mGroupVersion = reader.GetInteger("app", "group.version", 0);
  mAppVersion = reader.Get("app", "version", "v2");
  mStressTestEnabled = reader.GetBoolean("app", "stress.test.enabled", false);
  SPDLOG_INFO("Global settings: subsystem.id={}, "
              "group.id={}, "
              "group.version={}, "
              "stress.test.enabled={}, "
              "app.version={}"
              "app.clusterid={}"
              "app.nodeid={}",
              mSubsystemId,
              mMyClusterId,
              mGroupVersion,
              mStressTestEnabled,
              mAppVersion,
              mMyClusterId,
              mMyNodeId);
}

bool AppInfo::parseAllClusters(const INIReader &iniReader) {
  /// load from local config, the cluster id and node id must be specified
  auto clusterConf = iniReader.Get("cluster", "cluster.conf", "");
  mAllClusters = parseToClusterInfo(clusterConf);
  mMyClusterId = iniReader.GetInteger("cluster", "self.clusterId", 0);
  mMyNodeId = iniReader.GetInteger("cluster", "self.nodeId", 0);
  bool hasMe = false;
  for (auto &[clusterId, info] : mAllClusters) {
    if (mMyClusterId == clusterId) {
      for (auto &[nodeId, node] : info.getAllNodes()) {
        if (nodeId == mMyNodeId) {
          hasMe = true;
          break;
        }
      }
    }
  }
  assert(hasMe);

  Signal::hub.handle<RouteSignal>([](const Signal &s) {
    const auto &signal = dynamic_cast<const RouteSignal &>(s);
    SPDLOG_WARN("for non-etcd controlled cluster direct start raft");
    signal.passValue(true);
  });

  SPDLOG_INFO("read raft cluster conf from local, "
              "cluster.conf={}, self.clusterId={}, self.nodeId={}",
              clusterConf, mMyClusterId, mMyNodeId);
  return true;
}

std::unordered_map<ClusterId, Cluster> AppInfo::parseToClusterInfo(const std::string &infoStr) const {
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

}  /// namespace gringofts::app

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

#include "ClusterInfo.h"

#include <absl/strings/str_split.h>
#include <absl/strings/str_format.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <spdlog/spdlog.h>

#include "Util.h"

namespace gringofts {

std::string ClusterInfo::to_string() const {
  std::string str;
  for (const auto &kv : mNodes) {
    auto idx = kv.first;
    auto node = kv.second;
    str += absl::StrFormat("%d@%s:%d|%d|%d|%d|%d|%d,",
                           idx, node.mHostName,
                           node.mPortForRaft, node.mPortForGateway, node.mPortForFetcher,
                           node.mPortForStream, node.mPortForNetAdmin, node.mPortForScale);
  }
  return str;
}

bool ClusterInfo::checkHasRoute(const std::string &routeStr, uint64_t clusterId, uint64_t epoch) {
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


std::tuple<ClusterId, NodeId, std::map<ClusterId, ClusterInfo>> ClusterInfo::resolveAllClusters(
    const INIReader &iniReader, std::unique_ptr<kv::ClientFactory> factory) {
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
        for (auto &[nodeId, node] : info.mNodes) {
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
    std::string externalConfigFile = iniReader.Get("raft.external", "config.file", "");
    std::string clusterConfKey = iniReader.Get("raft.external", "cluster.conf.key", "");
    std::string clusterRouteKey = iniReader.Get("raft.external", "cluster.route.key", "");
    assert(!externalConfigFile.empty());
    assert(!clusterConfKey.empty());

    /// init external client
    auto client = factory->produce(INIReader(externalConfigFile));
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
      for (auto &[nodeId, node] : info.mNodes) {
        if (myHostname == node.mHostName) {
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
      signal.passValue(ClusterInfo::checkHasRoute(val, clusterId, signal.mEpoch));
    });

    return {*myClusterId, *myNodeId, allClusterInfo};
  }
}

std::map<ClusterId, ClusterInfo> ClusterInfo::parseToClusterInfo(const std::string &infoStr) {
  std::map<ClusterId, ClusterInfo> result;
  std::vector<std::string> clusters = absl::StrSplit(infoStr, ";");
  for (auto &c : clusters) {
    ClusterInfo info;
    std::pair<std::string, std::string> clusterIdWithNodes = absl::StrSplit(c, "#");
    info.mClusterId = std::stoi(clusterIdWithNodes.first);
    std::vector<std::string> nodes = absl::StrSplit(clusterIdWithNodes.second, ",");
    for (auto &n : nodes) {
      Node node;
      std::pair<std::string, std::string> hostWithPort = absl::StrSplit(n, ":");
      std::pair<std::string, std::string> idWithHost = absl::StrSplit(hostWithPort.first, "@");
      node.mNodeId = std::stoi(idWithHost.first);
      node.mHostName = idWithHost.second;
      if (hostWithPort.second.empty()) {
        SPDLOG_INFO("{} no specific port, using default one", hostWithPort.second);
      } else {
        std::vector<std::string> ports = absl::StrSplit(hostWithPort.second, "|");
        assert(ports.size() == 6);
        node.mPortForRaft = std::stoi(ports[0]);
        node.mPortForGateway = std::stoi(ports[1]);
        node.mPortForFetcher = std::stoi(ports[2]);
        node.mPortForStream = std::stoi(ports[3]);
        node.mPortForNetAdmin = std::stoi(ports[4]);
        node.mPortForScale = std::stoi(ports[5]);
      }
      info.addNode(node);
    }

    result[info.mClusterId] = info;
  }
  return result;
}

}  /// namespace gringofts

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
#include <absl/strings/str_join.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <spdlog/spdlog.h>

#include "Util.h"

namespace gringofts {

std::string ClusterInfo::to_string() const {
  std::string str;
  // mClusterId
  str += "cluster id: " + std::to_string(mClusterId) + ";";
  // mNodes
  if (mNodes.empty()) {
    str += "no members";
  } else {
    for (const auto &kv : mNodes) {
      auto idx = kv.first;
      auto node = kv.second;
      str += absl::StrFormat("%d@%s:%d|%d|%d|%d|%d|%d|InitialRole:%d, ",
                            idx, node.mHostName,
                            node.mPortForRaft, node.mPortForGateway, node.mPortForFetcher,
                            node.mPortForStream, node.mPortForNetAdmin, node.mPortForCtrl,
                            node.mInitialRole);
    }
  }
  str += ";";
  // mNewJointMembers and mOldJointMembers
  if (!hasJointMembers()) {
    str += "no joint members";
  } else {
    str += "new joint members:" + absl::StrJoin(mNewJointMembers, ",") + ";";
    str += "old joint members:" + absl::StrJoin(mOldJointMembers, ",") + ";";
  }
  str += ";";
  if (hasInsyncLearners()) {
    str += "insync learners:" + absl::StrJoin(mInsyncLearners, ",") + ";";
  }
  return str;
}

std::tuple<ClusterId, NodeId, uint64_t, std::map<ClusterId, ClusterInfo>> ClusterInfo::resolveAllClusters(
    const INIReader &iniReader, uint64_t clusterVersionFromState, const ClusterInfo &clusterInfoFromState,
    std::unique_ptr<kv::ClientFactory> factory) {
  std::string storeType = iniReader.Get("cluster", "persistence.type", "UNKNOWN");
  assert(storeType == "raft");
  bool externalEnabled = iniReader.GetBoolean("raft.external", "enable", false);
  if (!externalEnabled) {
    /// load from local config, the cluster id and node id must be specified
    auto clusterConf = iniReader.Get("cluster", "cluster.conf", "");
    auto learnersConf = iniReader.Get("cluster", "cluster.conf.learners", "");
    auto allClusterInfo = parseToClusterInfo(clusterConf, learnersConf);
    auto myClusterId = iniReader.GetInteger("cluster", "self.clusterId", -1);
    auto myNodeId = iniReader.GetInteger("cluster", "self.nodeId", -1);
    auto clusterVersion = iniReader.GetInteger("cluster", "cluster.conf.version", 0);
    if (myClusterId != -1 && myNodeId != -1) {
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
    } else {
      std::string myHostname;
      const char* selfUdns = getenv("SELF_UDNS");
      if (selfUdns == nullptr) {
        myHostname = Util::getHostname();
      } else {
        myHostname = selfUdns;
      }
      SPDLOG_INFO("SELF HOSTNAME: {}", myHostname);
      for (auto &[clusterId, info] : allClusterInfo) {
        for (auto &[nodeId, node] : info.mNodes) {
          if (myHostname == node.mHostName) {
            myClusterId = clusterId;
            myNodeId = nodeId;
            break;
          }
        }
      }
      assert(myClusterId != -1);
      assert(myNodeId != -1);
    }

    Signal::hub.handle<RouteSignal>([](const Signal &s) {
      const auto &signal = dynamic_cast<const RouteSignal &>(s);
      SPDLOG_WARN("for non-external controlled cluster direct start raft");
      signal.passValue(true);
    });

    SPDLOG_INFO("read raft cluster conf from local, "
                "cluster.conf={}, cluster.conf.learners={}, self.clusterId={}, self.nodeId={}",
                clusterConf, learnersConf, myClusterId, myNodeId);
    return {myClusterId, myNodeId, clusterVersion, allClusterInfo};
  } else {
    std::string externalConfigFile = iniReader.Get("raft.external", "config.file", "");
    std::string clusterConfKey = iniReader.Get("raft.external", "cluster.conf.key", "");
    std::string clusterRouteKey = iniReader.Get("raft.external", "cluster.route.key", "");
    static const std::string learnerKeySuffix(".learners");
    static const std::string versionKeySuffix(".version");
    assert(!externalConfigFile.empty());
    assert(!clusterConfKey.empty());

    /// init external client
    auto client = factory->produce(INIReader(externalConfigFile));
    /// read raft cluster conf from external
    std::string clusterConf;
    auto r = client->getValue(clusterConfKey, &clusterConf);
    assert(!clusterConfKey.empty());
    std::string learnersConf;
    r = client->getValue(clusterConfKey + learnerKeySuffix, &learnersConf);
    auto allClusterInfo = parseToClusterInfo(clusterConf, learnersConf);
    /// read raft cluster conf version from external
    std::string clusterVersionStr;
    r = client->getValue(clusterConfKey + versionKeySuffix, &clusterVersionStr);
    uint64_t clusterVersionFromExternal = clusterVersionStr.empty() ? 0 : std::stoi(clusterVersionStr);
    SPDLOG_INFO("raft cluster conf passed from external, "
                "cluster.version={}, cluster.conf={}, cluster.conf.learners={}",
                clusterVersionFromExternal, clusterConf, learnersConf);

    /// read raft cluster conf version from state
    uint64_t clusterVersion = clusterVersionFromExternal;
    /// use the larger version
    if (clusterVersionFromState > clusterVersionFromExternal) {
      clusterVersion = clusterVersionFromState;
      allClusterInfo[clusterInfoFromState.getClusterId()] = clusterInfoFromState;
      SPDLOG_INFO("raft cluster conf updated from ctrl state, cluster.version={}, cluster.conf={}",
                  clusterVersionFromState, clusterInfoFromState.to_string());
    }
    /// N.B.: when using external, the assumption is two PUs will never run on the same host,
    ///       otherwise below logic will break.
    std::optional<ClusterId> myClusterId = std::nullopt;
    NodeId myNodeId = kUnknownNodeId;
    std::string myHostname;
    for (auto &[clusterId, info] : allClusterInfo) {
      if (info.deduceSelfNodeId(&myNodeId, &myHostname)) {
          myClusterId = clusterId;
          break;
        }
      }
    if (!myClusterId) {
      SPDLOG_ERROR("My cluster id is unknown, please check the cluster configuration.");
      throw std::runtime_error("My cluster id is unknown, please check the cluster configuration.");
    }
    if (myNodeId == kUnknownNodeId) {
      SPDLOG_ERROR("My node id is unknown, please check the cluster configuration.");
      throw std::runtime_error("My node id is unknown, please check the cluster configuration.");
    }
    SPDLOG_INFO("raft cluster conf initiated. my cluster id: {}, my node id: {}, my hostname: {}",
                 *myClusterId, myNodeId, myHostname);
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

    return {*myClusterId, myNodeId, clusterVersion, allClusterInfo};
  }
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

std::map<ClusterId, ClusterInfo> ClusterInfo::parseToClusterInfo(const std::string &infoStr,
                                                                 const std::string &learnersStr) {
  std::map<ClusterId, ClusterInfo> result;

  // parse cluster infoStr
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
        node.mPortForCtrl = std::stoi(ports[5]);
      }
      node.mInitialRole = InitialRaftRole::Voter;
      info.addNode(node);
    }
    result[info.mClusterId] = info;
  }

  // parse learnerStr
  if (!learnersStr.empty()) {
    std::vector<std::string> clusterLearnerIds = absl::StrSplit(learnersStr, ";");
    for (auto &c : clusterLearnerIds) {
      std::pair<std::string, std::string> clusterIdWithLearnerIds = absl::StrSplit(c, "#");
      gringofts::ClusterId clusterId = std::stoi(clusterIdWithLearnerIds.first);
      if (result.find(clusterId) == result.end()) {
        SPDLOG_ERROR("Unknown cluser id {} in learner config: {}", clusterId, learnersStr);
        throw std::runtime_error("Unknown cluser id in learner config");
      }

      std::set<std::string> learnerIdStrs = absl::StrSplit(clusterIdWithLearnerIds.second, ",");

      auto &clusterInfo = result[clusterId];
      for (auto &learnerIdStr : learnerIdStrs) {
        NodeId learnerId = std::stoi(learnerIdStr);

        if (clusterInfo.mNodes.find(learnerId) == clusterInfo.mNodes.end()) {
          SPDLOG_ERROR("Unknown node id {} in cluster {} in learner config: {}", learnerIdStr, clusterId, learnersStr);
          throw std::runtime_error("Unknown node id in learner config");
        }
        clusterInfo.mNodes[learnerId].mInitialRole = InitialRaftRole::Learner;
      }
    }
  }

  return result;
}

bool ClusterInfo::deduceSelfNodeId(NodeId *myNodeId, std::string *myHostname) const {
  *myNodeId = kUnknownNodeId;
  *myHostname = "";

  const char* selfUdns = getenv("SELF_UDNS");
  if (selfUdns == nullptr) {
    *myHostname = Util::getHostname();
  } else {
    *myHostname = selfUdns;
  }
  SPDLOG_INFO("SELF HOSTNAME {}", *myHostname);

  for (auto &[nodeId, node] : mNodes) {
    if (*myHostname == node.mHostName) {
      *myNodeId = nodeId;
      return true;
    }
  }

  return false;
}

void ClusterInfo::syncToProto(ClusterInfoProto *proto) const {
  proto->set_clusterid(mClusterId);
  proto->clear_members();
  for (const auto &node : mNodes) {
    auto *memberProto = proto->add_members();
    memberProto->set_id(node.first);
    memberProto->set_address(node.second.mHostName);
    memberProto->set_port(node.second.mPortForRaft);
    memberProto->set_role(node.second.mInitialRole);
  }
  proto->mutable_jointmembers()->clear_oldmemberids();
  proto->mutable_jointmembers()->clear_newmemberids();
  if (hasJointMembers()) {
    auto *jointMembers = proto->mutable_jointmembers();
    for (const auto &nodeId : mOldJointMembers) {
      jointMembers->add_oldmemberids(nodeId);
    }
    for (const auto &nodeId : mNewJointMembers) {
      jointMembers->add_newmemberids(nodeId);
    }
  }
  proto->clear_insynclearners();
  if (hasInsyncLearners()) {
    for (const auto &nodeId : mInsyncLearners) {
      proto->add_insynclearners(nodeId);
    }
  }
}

void ClusterInfo::syncFromProto(const ClusterInfoProto &proto) {
  mClusterId = proto.clusterid();
  mNodes.clear();
  mOldJointMembers.clear();
  mNewJointMembers.clear();
  mInsyncLearners.clear();
  for (const auto &member : proto.members()) {
    Node node;
    node.mNodeId = member.id();
    node.mHostName = member.address();
    node.mPortForRaft = member.port() == 0 ? kDefaultRaftPort : member.port();
    node.mInitialRole = member.role();
    addNode(node);
  }
  for (const auto &nodeId : proto.jointmembers().oldmemberids()) {
    addOldJointMember(nodeId);
  }
  for (const auto &nodeId : proto.jointmembers().newmemberids()) {
    addNewJointMember(nodeId);
  }
  for (const auto &nodeId : proto.insynclearners()) {
    addInsyncLearner(nodeId);
  }
}

}  /// namespace gringofts

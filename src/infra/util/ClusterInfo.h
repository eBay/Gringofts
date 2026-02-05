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

#ifndef SRC_INFRA_UTIL_CLUSTERINFO_H_
#define SRC_INFRA_UTIL_CLUSTERINFO_H_

#include <INIReader.h>
#include <map>
#include <set>

#include "KVClient.h"
#include "../../app_util/generated/grpc/ctrl.pb.h"
#include "Signal.h"

namespace gringofts {

using ClusterId = uint32_t;
using NodeId = uint32_t;
using HostName = std::string;
using Port = uint32_t;
using ClusterInfoProto = gringofts::app::protos::ClusterConfiguration;
using app::protos::InitialRaftRole;

static constexpr NodeId kUnknownNodeId = 0;

static constexpr Port kDefaultRaftPort = 5254;
static constexpr Port kDefaultGatewayPort = 50055;
static constexpr Port kDefaultFetcherPort = 50056;
static constexpr Port kDefaultStreamingPort = 5678;
static constexpr Port kDefaultNetAdminPort = 50065;
static constexpr Port kDefaultCtrlPort = 61203;

struct RouteSignal : public FutureSignal<bool> {
  RouteSignal(uint64_t epoch, uint64_t clusterId) : mEpoch(epoch), mClusterId(clusterId) {}
  uint64_t mEpoch;
  uint64_t mClusterId;
};

class ClusterInfo final {
 public:
  struct Node {
    NodeId mNodeId;
    HostName mHostName;
    Port mPortForRaft = kDefaultRaftPort;
    Port mPortForGateway = kDefaultGatewayPort;
    Port mPortForFetcher = kDefaultFetcherPort;
    Port mPortForStream = kDefaultStreamingPort;
    Port mPortForNetAdmin = kDefaultNetAdminPort;
    Port mPortForCtrl = kDefaultCtrlPort;
    InitialRaftRole mInitialRole = InitialRaftRole::Voter;

    bool operator==(Node const &other) const {
      return mNodeId == other.mNodeId &&
             mHostName == other.mHostName &&
             mPortForRaft == other.mPortForRaft &&
             mPortForGateway == other.mPortForGateway &&
             mPortForFetcher == other.mPortForFetcher &&
             mPortForStream == other.mPortForStream &&
             mPortForNetAdmin == other.mPortForNetAdmin &&
             mPortForCtrl == other.mPortForCtrl &&
             mInitialRole == other.mInitialRole;
    }
  };

  /**
   * Return my cluster id and full cluster info
   */
  static std::tuple<ClusterId, NodeId, uint64_t, std::map<ClusterId, ClusterInfo>> resolveAllClusters(
      const INIReader &iniReader, uint64_t clusterVersionFromState, const ClusterInfo &clusterInfoFromState,
      std::unique_ptr<kv::ClientFactory> factory);

  static bool checkHasRoute(const std::string &routeStr, uint64_t clusterId, uint64_t epoch);

  inline void addNode(const Node &node) { mNodes[node.mNodeId] = node; }
  inline void addNewJointMember(const NodeId &nodeId) { mNewJointMembers.insert(nodeId); }
  inline void addOldJointMember(const NodeId &nodeId) { mOldJointMembers.insert(nodeId); }
  inline void addInsyncLearner(const NodeId &nodeId) { mInsyncLearners.insert(nodeId); }

  std::map<NodeId, Node> getAllNodeInfo() const { return mNodes; }

  const std::map<NodeId, InitialRaftRole> getAllInitialRoles() const {
    std::map<NodeId, InitialRaftRole> res;
    for (const auto node : mNodes) {
      res[node.first] = node.second.mInitialRole;
    }
    return res;
  }

  bool hasJointMembers() const { return !mOldJointMembers.empty() && !mNewJointMembers.empty(); }
  const std::set<NodeId>& getOldJointMembers() const { return mOldJointMembers; }
  const std::set<NodeId>& getNewJointMembers() const { return mNewJointMembers; }
  bool hasInsyncLearners() const { return !mInsyncLearners.empty(); }
  const std::set<NodeId>& getInsyncLearners() const { return mInsyncLearners; }

  void setClusterId(ClusterId clusterId) {
    mClusterId = clusterId;
  }

  ClusterId getClusterId() const {
    return mClusterId;
  }

  // ture: selfnode is in the cluster, false: selfnode is not in the cluster
  bool deduceSelfNodeId(NodeId *myNodeId, std::string *myHostname) const;

  std::string to_string() const;
  void syncToProto(ClusterInfoProto *proto) const;
  void syncFromProto(const ClusterInfoProto &proto);

 private:
  /// example with two three-node clusters:
  /// @param infoStr
  /// @example 0#1@node01.ebay.com:5245|50055|50056|5678|50065|61203,
  /// 2@node02.ebay.com:5245|50055|50056|5678|50065|61203,
  /// 3@node03.ebay.com:5245|50055|50056|5678|50065|61203;
  /// 1#1@node11.ebay.com:5245|50055|50056|5678|50065|61203,2@node12.ebay.com:5245|50055|50056|5678|50065|61203,
  /// 3@node13.ebay.com:5245|50055|50056|5678|50065|61203
  /// @param learnersStr
  /// @example 0#1,2,3;2#2,3
  static std::map<ClusterId, ClusterInfo> parseToClusterInfo(const std::string &infoStr,
      const std::string &learnersStr);

  ClusterId mClusterId;
  std::map<NodeId, Node> mNodes;
  std::set<NodeId> mOldJointMembers;  // if no joint group, it's empty
  std::set<NodeId> mNewJointMembers;  // if no joint group, it's empty
  std::set<NodeId> mInsyncLearners;   // if no in sync learner, it's empty
};
}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_CLUSTERINFO_H_

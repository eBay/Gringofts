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

#include "KVClient.h"
#include "Signal.h"

namespace gringofts {

using ClusterId = uint32_t;
using NodeId = uint32_t;
using HostName = std::string;
using Port = uint32_t;

static constexpr Port kDefaultRaftPort = 5254;
static constexpr Port kDefaultGatewayPort = 50055;
static constexpr Port kDefaultFetcherPort = 50056;
static constexpr Port kDefaultStreamingPort = 5678;
static constexpr Port kDefaultNetAdminPort = 50065;
static constexpr Port kDefaultScalePort = 61203;

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
    Port mPortForScale = kDefaultScalePort;
  };

  /**
   * Return my cluster id and full cluster info
   */
  static std::tuple<ClusterId, NodeId, std::map<ClusterId, ClusterInfo>> resolveAllClusters(
      const INIReader &iniReader, std::unique_ptr<kv::ClientFactory> factory);

  static bool checkHasRoute(const std::string &routeStr, uint64_t clusterId, uint64_t epoch);

  inline void addNode(const Node &node) { mNodes[node.mNodeId] = node; }

  std::map<NodeId, Node> getAllNodeInfo() const { return mNodes; }

  std::string to_string() const;

 private:
  /// example with two three-node clusters:
  /// 0#1@node01.ebay.com:5245|50055|50056|5678|50065|61203,2@node02.ebay.com:5245|50055|50056|5678|50065|61203,
  /// 3@node03.ebay.com:5245|50055|50056|5678|50065|61203;
  /// 1#1@node11.ebay.com:5245|50055|50056|5678|50065|61203,2@node12.ebay.com:5245|50055|50056|5678|50065|61203,
  /// 3@node13.ebay.com:5245|50055|50056|5678|50065|61203
  static std::map<ClusterId, ClusterInfo> parseToClusterInfo(const std::string &infoStr);

  ClusterId mClusterId;
  std::map<NodeId, Node> mNodes;
};
}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_CLUSTERINFO_H_

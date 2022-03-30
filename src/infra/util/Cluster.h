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

#ifndef SRC_INFRA_UTIL_CLUSTER_H_
#define SRC_INFRA_UTIL_CLUSTER_H_

#include <memory>
#include <unordered_map>
#include <string>

#include <INIReader.h>

namespace gringofts {

using ClusterId = uint32_t;
using NodeId = uint32_t;
using HostName = std::string;
using PortType = uint32_t;
using Port = uint32_t;

class Node {
 public:
  Node(NodeId id, HostName hostName) : mNodeId(id), mHostName(std::move(hostName)) {}
  inline NodeId id() const { return mNodeId; }
  inline HostName hostName() const { return mHostName; }

  virtual std::string to_string() const;
 private:
  NodeId mNodeId;
  HostName mHostName;
};

class RaftNode : public Node {
  static constexpr Port kDefaultStreamingPort = 5678;
  static constexpr Port kDefaultRaftPort = 5254;
 public:
  RaftNode(NodeId id, HostName hostName) : Node(id, std::move(hostName)) {}
  RaftNode(NodeId id, HostName hostName, Port raftPort, Port streamPort)
      : Node(id, std::move(hostName)), mPortForRaft(raftPort), mPortForStream(streamPort) {}

  inline Port streamPort() const { return mPortForStream; }
  inline Port raftPort() const { return mPortForRaft; }
 private:
  Port mPortForStream = kDefaultStreamingPort;
  Port mPortForRaft = kDefaultRaftPort;
};

class Cluster {
 public:
  inline void setClusterId(ClusterId id) { mClusterId = id; }
  inline void addNode(std::shared_ptr<Node> node) { mNodes[node->id()] = std::move(node); }

  inline ClusterId id() const { return mClusterId; }
  inline std::unordered_map<NodeId, std::shared_ptr<Node>> getAllNodes() const { return mNodes; }

  std::string to_string() const;
 private:
  ClusterId mClusterId;
  std::unordered_map<NodeId, std::shared_ptr<Node>> mNodes;
};

class ClusterParser {
 public:
  using ClusterMap = std::unordered_map<ClusterId, Cluster>;
  virtual ~ClusterParser() = default;
  virtual std::tuple<NodeId, ClusterId, ClusterMap> parse(const INIReader &) = 0;
};
}  // namespace gringofts
#endif  // SRC_INFRA_UTIL_CLUSTER_H_

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

#ifndef SRC_APP_UTIL_APPINFO_H_
#define SRC_APP_UTIL_APPINFO_H_

#include <INIReader.h>

#include "../infra/common_types.h"
#include "../infra/util/Cluster.h"
#include "../infra/util/Signal.h"
#include "../infra/util/KVClient.h"

namespace gringofts {
namespace app {

struct RouteSignal : public FutureSignal<bool> {
  RouteSignal(uint64_t epoch, uint64_t clusterId) : mEpoch(epoch), mClusterId(clusterId) {}
  uint64_t mEpoch;
  uint64_t mClusterId;
};

class AppNode : public RaftNode {
  static constexpr Port kDefaultGatewayPort = 50055;
  static constexpr Port kDefaultFetchPort = 50056;
  static constexpr Port kDefaultNetAdminPort = 50065;
  static constexpr Port kDefaultScalePort = 61203;
 public:
  AppNode(NodeId id, const HostName &hostName) : RaftNode(id, hostName) {}
  AppNode(NodeId id, const HostName &hostName,
          Port raftPort, Port streamPort,
          Port gateWayPort, Port dumperPort, Port netAdminPort, Port scalePort)
      : RaftNode(id, hostName, raftPort, streamPort),
        mPortForGateway(gateWayPort), mPortForFetch(dumperPort),
        mPortForNetAdmin(netAdminPort), mPortForScale(scalePort) {
  }
  inline Port gateWayPort() const { return mPortForGateway; }
  inline Port fetchPort() const { return mPortForFetch; }
  inline Port netAdminPort() const { return mPortForNetAdmin; }
  inline Port scalePort() const { return mPortForScale; }
 private:
  Port mPortForGateway = kDefaultGatewayPort;
  Port mPortForFetch = kDefaultFetchPort;
  Port mPortForNetAdmin = kDefaultNetAdminPort;
  Port mPortForScale = kDefaultScalePort;
};

class AppClusterParser : public ClusterParser {
 public:
  AppClusterParser() : mKvFactory(nullptr) {}
  explicit AppClusterParser(std::unique_ptr<kv::ClientFactory> factory) : mKvFactory(std::move(factory)) {}
  std::tuple<NodeId, ClusterId, ClusterMap> parse(const INIReader &) override;

  static bool checkHasRoute(const std::string &routeStr, uint64_t clusterId, uint64_t epoch);

 private:
  std::unordered_map<ClusterId, Cluster> parseToClusterInfo(const std::string &infoStr) const;

  std::unique_ptr<kv::ClientFactory> mKvFactory;
};

class AppInfo final {
 public:
  ~AppInfo() = default;

  static void reload() {
    getInstance().initialized = false;
  }

  static void init(const INIReader &reader,
                   std::unique_ptr<ClusterParser> parser = std::make_unique<AppClusterParser>());

  /// disallow copy ctor and copy assignment
  AppInfo(const AppInfo &) = delete;
  AppInfo &operator=(const AppInfo &) = delete;

  static Id subsystemId() { return getInstance().mSubsystemId; }
  static Id groupId() { return getInstance().mMyClusterId; }
  static uint64_t groupVersion() { return getInstance().mGroupVersion; }
  static bool stressTestEnabled() { return getInstance().mStressTestEnabled; }
  static std::string appVersion() { return getInstance().mAppVersion; }

  static Cluster getMyClusterInfo() {
    return getInstance().mAllClusterInfo[getInstance().mMyClusterId];
  }

  static std::shared_ptr<AppNode> getMyNode() {
    assert(getInstance().initialized);
    return std::dynamic_pointer_cast<AppNode>(getMyClusterInfo().getAllNodes()[getMyNodeId()]);
  }

  static std::optional<Cluster> getClusterInfo(uint64_t clusterId) {
    if (getInstance().mAllClusterInfo.count(clusterId)) {
      return getInstance().mAllClusterInfo[clusterId];
    } else {
      return std::nullopt;
    }
  }
  static NodeId getMyNodeId() { return getInstance().mMyNodeId; }

  static ClusterId getMyClusterId() { return getInstance().mMyClusterId; }

  static Port netAdminPort() {
    return getMyNode()->netAdminPort();
  }

  static Port scalePort() {
    return getMyNode()->scalePort();
  }

  static Port gatewayPort() {
    return getMyNode()->gateWayPort();
  }

  static Port fetchPort() {
    return getMyNode()->fetchPort();
  }

 private:
  AppInfo() = default;

  static AppInfo &getInstance() {
    static AppInfo appInfo;
    return appInfo;
  }

  inline void setSubsystemId(Id id) {
    assert(id > 0);
    mSubsystemId = id;
  }

  inline void setGroupId(Id id) { mMyClusterId = id; }

  inline void setGroupVersion(uint64_t version) { mGroupVersion = version; }

  inline void enableStressTest(bool enabled) { mStressTestEnabled = enabled; }

  inline void setAppVersion(const std::string &appVersion) { mAppVersion = appVersion; }

  std::atomic<bool> initialized = false;
  /**
   * Uniquely identifies the system
   * Each version that is not backward-compatible should have a different id
   */
  Id mSubsystemId = 0;
  /**
   * Each creator can belong to a different partition under different version
   */
  uint64_t mGroupVersion = 0;
  /**
   * True if current app is for stress test. Default is false.
   */
  bool mStressTestEnabled = false;
  /**
   * App version
   */
  std::string mAppVersion = "v2";
  /**
   * Cluster Info
   */
  std::unordered_map<ClusterId, Cluster> mAllClusterInfo;
  ClusterId mMyClusterId;
  NodeId mMyNodeId;
};

}  /// namespace app
}  /// namespace gringofts

#endif  // SRC_APP_UTIL_APPINFO_H_

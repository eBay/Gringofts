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

class AppInfo : public GroupInfoInterface {
 public:
  ~AppInfo() = default;

  explicit AppInfo(const INIReader &reader);

  bool parseAllClusters(const INIReader &) override;

  /// disallow copy ctor and copy assignment
  AppInfo(const AppInfo &) = delete;
  AppInfo &operator=(const AppInfo &) = delete;

  inline Id subsystemId() const { return mSubsystemId; }
  inline Id groupId() const { return mMyClusterId; }
  inline uint64_t groupVersion() const { return mGroupVersion; }
  inline std::string appVersion() const { return mAppVersion; }

  // port expose
  inline Port gateWayPort() const { return std::dynamic_pointer_cast<AppNode>(getMyNode())->gateWayPort();}
  inline Port fetchPort() const { return std::dynamic_pointer_cast<AppNode>(getMyNode())->fetchPort();}
  inline Port netAdminPort() const { return std::dynamic_pointer_cast<AppNode>(getMyNode())->netAdminPort();}

 private:
  std::unordered_map<ClusterId, Cluster> parseToClusterInfo(const std::string &infoStr) const;
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
};

}  /// namespace app
}  /// namespace gringofts

#endif  // SRC_APP_UTIL_APPINFO_H_

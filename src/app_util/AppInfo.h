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
#include "../infra/util/ClusterInfo.h"

namespace gringofts {
namespace app {

class AppInfo final {
 public:
  ~AppInfo() = default;

  static void reload() {
    getInstance().initialized = false;
  }

  static void init(const INIReader &reader);

  /// disallow copy ctor and copy assignment
  AppInfo(const AppInfo &) = delete;
  AppInfo &operator=(const AppInfo &) = delete;

  static Id subsystemId() { return getInstance().mSubsystemId; }
  static Id groupId() { return getInstance().mMyClusterId; }
  static uint64_t groupVersion() { return getInstance().mGroupVersion; }
  static bool stressTestEnabled() { return getInstance().mStressTestEnabled; }
  static std::string appVersion() { return getInstance().mAppVersion; }

  static ClusterInfo getMyClusterInfo() {
    return getInstance().mAllClusterInfo[getInstance().mMyClusterId];
  }

  static ClusterInfo::Node getMyNode() {
    assert(getInstance().initialized);
    return getMyClusterInfo().getAllNodeInfo()[getMyNodeId()];
  }

  static std::optional<ClusterInfo> getClusterInfo(uint64_t clusterId) {
    if (getInstance().mAllClusterInfo.count(clusterId)) {
      return getInstance().mAllClusterInfo[clusterId];
    } else {
      return std::nullopt;
    }
  }
  static NodeId getMyNodeId() { return getInstance().mMyNodeId; }

  static ClusterId getMyClusterId() { return getInstance().mMyClusterId; }

  static Port netAdminPort() {
    auto node = getMyClusterInfo().getAllNodeInfo()[getMyNodeId()];
    return node.mPortForNetAdmin;
  }

  static Port scalePort() {
    return getMyNode().mPortForScale;
  }

  static Port gatewayPort() {
    return getMyNode().mPortForGateway;
  }

  static Port fetchPort() {
    return getMyNode().mPortForFetcher;
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
  std::map<ClusterId, ClusterInfo> mAllClusterInfo;
  ClusterId mMyClusterId;
  NodeId mMyNodeId;
};

}  /// namespace app
}  /// namespace gringofts

#endif  // SRC_APP_UTIL_APPINFO_H_

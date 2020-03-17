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
#include <spdlog/spdlog.h>

#include "../infra/common_types.h"

namespace gringofts {
namespace app {

class AppInfo final {
 public:
  ~AppInfo() = default;

  static void init(const INIReader &reader) {
    auto &appInfo = getInstance();

    auto &initialized = appInfo.initialized;
    bool expected = false;

    if (!initialized.compare_exchange_strong(expected, true)) {
      SPDLOG_WARN("appInfo has already been initialized, ignore.");
      return;
    }

    appInfo.setSubsystemId(reader.GetInteger("app", "subsystem.id", 0));
    appInfo.setGroupId(reader.GetInteger("app", "group.id", 0));
    appInfo.setGroupVersion(reader.GetInteger("app", "group.version", 0));
    appInfo.enableStressTest(reader.GetBoolean("app", "stress.test.enabled", false));
    appInfo.setAppVersion(reader.Get("app", "version", "v2"));

    SPDLOG_INFO("Global settings: subsystem.id={}, "
                "group.id={}, "
                "group.version={}, "
                "stress.test.enabled={}, ",
                "app.version={}",
                appInfo.mSubsystemId,
                appInfo.mGroupId,
                appInfo.mGroupVersion,
                appInfo.mStressTestEnabled,
                appInfo.mAppVersion);
  }

  /// disallow copy ctor and copy assignment
  AppInfo(const AppInfo &) = delete;
  AppInfo &operator=(const AppInfo &) = delete;

  static Id subsystemId() { return getInstance().mSubsystemId; }
  static Id groupId() { return getInstance().mGroupId; }
  static uint64_t groupVersion() { return getInstance().mGroupVersion; }
  static bool stressTestEnabled() { return getInstance().mStressTestEnabled; }
  static std::string appVersion() { return getInstance().mAppVersion; }

 private:
  AppInfo() = default;

  static AppInfo &getInstance() {
    static AppInfo appInfo;
    return appInfo;
  }

  void setSubsystemId(Id id) {
    assert(id > 0);
    mSubsystemId = id;
  }

  void setGroupId(Id id) {
    mGroupId = id;
  }

  void setGroupVersion(uint64_t version) {
    mGroupVersion = version;
  }

  void enableStressTest(bool enabled) {
    mStressTestEnabled = enabled;
  }

  void setAppVersion(const std::string &appVersion) {
    mAppVersion = appVersion;
  }

  std::atomic<bool> initialized = false;
  /**
   * Uniquely identifies the system
   * Each version that is not backward-compatible should have a different id
   */
  Id mSubsystemId = 0;
  /**
   * Identify the partition (i.e., group) that the creator belongs to
   */
  Id mGroupId = 0;
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

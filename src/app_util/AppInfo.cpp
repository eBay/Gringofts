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

#include <spdlog/spdlog.h>

#include "AppInfo.h"

namespace gringofts::app {

void AppInfo::init(const INIReader &reader) {
  auto &appInfo = getInstance();

  auto &initialized = appInfo.initialized;
  bool expected = false;

  if (!initialized.compare_exchange_strong(expected, true)) {
    SPDLOG_WARN("appInfo has already been initialized, ignore.");
    return;
  }

  auto[myClusterId, myNodeId, allClusterInfo] = ClusterInfo::resolveAllClusters(reader, nullptr);
  appInfo.mMyClusterId = myClusterId;
  appInfo.mMyNodeId = myNodeId;
  appInfo.mAllClusterInfo = allClusterInfo;

  appInfo.setSubsystemId(reader.GetInteger("app", "subsystem.id", 0));
  appInfo.setGroupId(appInfo.mMyClusterId);
  appInfo.setGroupVersion(reader.GetInteger("app", "group.version", 0));
  appInfo.enableStressTest(reader.GetBoolean("app", "stress.test.enabled", false));
  appInfo.setAppVersion(reader.Get("app", "version", "v2"));

  SPDLOG_INFO("Global settings: subsystem.id={}, "
              "group.id={}, "
              "group.version={}, "
              "stress.test.enabled={}, "
              "app.version={}"
              "app.clusterid={}"
              "app.nodeid={}",
              appInfo.mSubsystemId,
              appInfo.mMyClusterId,
              appInfo.mGroupVersion,
              appInfo.mStressTestEnabled,
              appInfo.mAppVersion,
              appInfo.mMyClusterId,
              appInfo.mMyNodeId);
}
}  /// namespace gringofts::app

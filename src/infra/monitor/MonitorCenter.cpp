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

#include "MonitorCenter.h"

#include <zconf.h>

#include "Monitorable.h"

namespace gringofts {

MonitorCenter::MonitorCenter() {
  running = true;
  mMonitorThread = std::thread(&MonitorCenter::run, this);
}

void MonitorCenter::registry(std::shared_ptr<Monitorable> monitorable) {
  std::lock_guard<std::mutex> lock_guard(mMutex);
  mMonitorables.push_back(monitorable);
}

void MonitorCenter::reportGaugeMetrics() {
  std::lock_guard<std::mutex> lock(mMutex);
  for (auto &monitorable : mMonitorables) {
    auto &metricTags = monitorable->monitorTags();
    for (auto &tag : metricTags) {
      gauge(tag.name, tag.labels).set(monitorable->getValue(tag));
    }
  }
}

MonitorCenter::~MonitorCenter() {
  running = false;
  if (mMonitorThread.joinable()) {
    mMonitorThread.join();
  }
}

void MonitorCenter::run() {
  while (running) {
    usleep(1000 * 10);  /// us
    reportGaugeMetrics();
  }
}

}  /// namespace gringofts

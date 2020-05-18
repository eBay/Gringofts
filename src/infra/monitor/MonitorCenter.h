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

#ifndef SRC_INFRA_MONITOR_MONITORCENTER_H_
#define SRC_INFRA_MONITOR_MONITORCENTER_H_

#include <memory>
#include <mutex>
#include <thread>

#include "Monitorable.h"
#include "santiago/Server.h"

namespace gringofts {

class MonitorCenter : public santiago::MetricsCenter {
 public:
  MonitorCenter() {
    running = true;
    mMonitorThread = std::thread(&MonitorCenter::run, this);
  }
  virtual ~MonitorCenter() {
    running = false;
    if (mMonitorThread.joinable()) {
      mMonitorThread.join();
    }
  }
  void registry(std::shared_ptr<Monitorable> monitorable) {
    std::lock_guard<std::mutex> lock_guard(mMutex);
    mMonitorables.push_back(monitorable);
  }

 private:
  void run() {
    while (running) {
      usleep(1000 * 10);  /// us
      reportGaugeMetrics();
    }
  }
  void reportGaugeMetrics() {
    std::lock_guard<std::mutex> lock(mMutex);
    for (auto monitorable : mMonitorables) {
      auto &metricTags = monitorable->monitorTags();
      for (auto &tag : metricTags) {
        gauge(tag.name, tag.labels).set(monitorable->getValue(tag));
      }
    }
  }
  std::thread mMonitorThread;
  std::vector<std::shared_ptr<Monitorable>> mMonitorables;
  std::mutex mMutex;
  std::atomic_bool running;
};

inline void enableMonitorable(std::shared_ptr<Monitorable> monitorable) {
  Singleton<MonitorCenter>::getInstance().registry(monitorable);
}

}  /// namespace gringofts

#endif  // SRC_INFRA_MONITOR_MONITORCENTER_H_

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

#ifndef SRC_INFRA_MONITOR_SANTIAGO_METRICS_COLLECTOR_H_
#define SRC_INFRA_MONITOR_SANTIAGO_METRICS_COLLECTOR_H_

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <prometheus/metric.h>
#include <prometheus/registry.h>

namespace santiago {

class MetricsCategory;

class MetricsCollector {
 private:
  MetricsCollector();

 public:
  ~MetricsCollector();

  // return a singleton instance of this class
  static MetricsCollector &Instance();

  bool init();
  bool shutdown();

  std::shared_ptr<prometheus::Registry> &GetRegistry() {
    return registry;
  }

  // register new metrics category
  void registerMetricsCategory(MetricsCategory *metricsCat) {
    metricsCategories.push_back(metricsCat);
  }

  // register each metric by name
  template<typename T>
  void registerMetric(const std::string &name_, T &metric_) {  // NOLINT [runtime/references]
    std::lock_guard<std::mutex> g{holderLock};
    metricsHolder.insert(std::make_pair(name_, &metric_));
  }

  // find a metric by registered name
  template<typename T>
  T *findMetric(const std::string &name_) {
    auto got = metricsHolder.find(name_);
    if (got != metricsHolder.end()) {
      return static_cast<T *>(got->second);
    } else {
      return nullptr;
    }
  }

 private:
  std::mutex holderLock;

  std::unordered_map<std::string, void *> metricsHolder;

  std::shared_ptr<prometheus::Registry> registry;

  std::list<MetricsCategory *> metricsCategories;
};

}  /// namespace santiago

#endif  // SRC_INFRA_MONITOR_SANTIAGO_METRICS_COLLECTOR_H_

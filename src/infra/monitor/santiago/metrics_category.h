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

#ifndef SRC_INFRA_MONITOR_SANTIAGO_METRICS_CATEGORY_H_
#define SRC_INFRA_MONITOR_SANTIAGO_METRICS_CATEGORY_H_

#include <string>

#include <prometheus/registry.h>

namespace santiago {

class MetricsCollector;

class MetricsCategory {
 protected:
  explicit MetricsCategory(MetricsCollector &collector);  // NOLINT [runtime/references]

 public:
  virtual ~MetricsCategory() {}

  static MetricsCategory &RegisterToCollector(MetricsCollector &collector) {  // NOLINT [runtime/references]
    static MetricsCategory metrics = MetricsCategory(collector);
    return metrics;
  }

  const std::string version() {
    return "0.1";
  }

 protected:
  prometheus::Registry &registry;
};

}  /// namespace santiago

#endif  // SRC_INFRA_MONITOR_SANTIAGO_METRICS_CATEGORY_H_

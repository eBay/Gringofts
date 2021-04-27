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

#ifndef SRC_INFRA_UTIL_METRICREPORTER_H_
#define SRC_INFRA_UTIL_METRICREPORTER_H_

#include <stdint.h>

#include "../monitor/MonitorTypes.h"
#include "TimeUtil.h"

namespace gringofts {

class MetricReporter final {
 public:
  static void reportLatency(
      const std::string &metricName,
      TimestampInNanos start,
      TimestampInNanos end,
      bool reportSummary = false,
      bool reportDetail = false,
      bool reportLog = false) {
    if (start > end) {
      SPDLOG_WARN("invalid latency of {}, {} vs {}", metricName, start, end);
      return;
    }
    auto latency = (end - start) / 1000000.0;
    if (reportSummary) {
      auto summary = gringofts::getSummary(metricName, {});
      summary.observe(latency);
    }
    if (reportDetail) {
      auto gauge = gringofts::getGauge("detail_" + metricName, {});
      gauge.set(latency);
    }
    if (reportLog) {
      SPDLOG_INFO("latency report of {}: {}", metricName, latency);
    }
  }
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_METRICREPORTER_H_

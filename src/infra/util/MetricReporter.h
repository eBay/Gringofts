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
  static void reportLatencyInHistogram(
      const std::string &metricName,
      TimestampInNanos start,
      TimestampInNanos end,
      std::string goblin_ns = "",
      bool reportHistogram = true,
      bool reportDetail = false,
      bool reportLog = false) {
    static const prometheus::Histogram::BucketBoundaries bucketBoundaries = prometheus::Histogram::BucketBoundaries {
        0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9,
        1, 1.048576, 1.398101, 1.747626, 2.097151, 2.446676, 2.796201, 3.145726, 3.495251, 3.844776, 4.194304,
        5.592405, 6.990506, 8.388607, 9.786708,
        11.184809, 12.58291, 13.981011, 15.379112, 16.777216, 22.369621, 27.962026, 33.554431, 39.146836, 44.739241,
        50.331646, 55.924051, 61.516456, 67.108864, 89.478485,
        111.848106, 134.217727, 156.587348, 178.956969, 201.32659, 223.696211, 246.065832, 268.435456, 357.913941,
        447.392426, 536.870911, 626.349396, 715.827881, 805.306366, 894.784851, 984.263336,
        1073.741824, 1431.655765, 1789.569706, 2147.483647, 2505.397588, 2863.311529, 3221.22547, 3579.139411,
        3937.053352, 4294.967296, 5726.623061, 7158.278826, 8589.934591,
        10021.59036, 11453.24612, 12884.90189, 14316.55765, 15748.21342, 17179.86918, 22906.49225, 28633.11531,
        30000.0};

    if (start > end) {
      SPDLOG_WARN("invalid latency of {}, {} vs {}", metricName, start, end);
      return;
    }
    auto latency = (end - start) / 1000000.0;
    if (reportHistogram) {
      auto histogram = gringofts::getHistogram(metricName, {{"goblin_ns", goblin_ns}}, bucketBoundaries);
      histogram.observe(latency);
    }
    if (reportDetail) {
      auto gauge = gringofts::getGauge("detail_" + metricName, {{"goblin_ns", goblin_ns}});
      gauge.set(latency);
    }
    if (reportLog) {
      SPDLOG_INFO("latency report of {}: {}", metricName, latency);
    }
  }

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

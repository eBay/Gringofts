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

#ifndef SRC_INFRA_RAFT_METRICS_RAFTMETRICS_H_
#define SRC_INFRA_RAFT_METRICS_RAFTMETRICS_H_

#include "../../monitor/santiago/metrics_category.h"
#include "../../monitor/santiago/metrics_collector.h"

namespace gringofts {

using ::santiago::MetricsCategory;
using ::santiago::MetricsCollector;
using ::prometheus::Family;
using ::prometheus::Counter;
using ::prometheus::Gauge;

class RaftMetrics final : public MetricsCategory {
 public:
  ~RaftMetrics() = default;

  static MetricsCategory &RegisterToCollector(MetricsCollector *collector) {
    static MetricsCategory metrics = RaftMetrics(collector);
    return metrics;
  }

  static void reportGrpcError(int errorCode);

  static constexpr auto kCommittedLogCounterName = R"(committed_log_counter)";
  static constexpr auto kLeadershipGaugeName = R"(leadership_gauge)";

  static constexpr auto kGrpcErrorCounterName = R"(grpc_error_counter)";

 private:
  explicit RaftMetrics(MetricsCollector *collector);

  Family<prometheus::Counter> &mCommittedLogCounter;
  Family<prometheus::Gauge>   &mLeadershipGauge;
  Family<prometheus::Counter> &mGrpcErrorCounter;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_METRICS_RAFTMETRICS_H_

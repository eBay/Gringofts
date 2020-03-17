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

#include "RaftMetrics.h"

namespace {
using ::santiago::MetricsCollector;
}

namespace gringofts {

RaftMetrics::RaftMetrics(MetricsCollector *collector)
    : MetricsCategory(*collector),
      mCommittedLogCounter(prometheus::BuildCounter()
                                .Name(kCommittedLogCounterName)
                                .Register(registry)),
      mLeadershipGauge(prometheus::BuildGauge()
                                .Name(kLeadershipGaugeName)
                                .Register(registry)),
      mGrpcErrorCounter(prometheus::BuildCounter()
                                .Name(kGrpcErrorCounterName)
                                .Register(registry)) {
  collector->registerMetric(kCommittedLogCounterName,
                            mCommittedLogCounter.Add({{"status", "committed"}}));
  collector->registerMetric(kLeadershipGaugeName,
                            mLeadershipGauge.Add({{"status", "isLeader"}}));

  collector->registerMetric(kGrpcErrorCounterName, mGrpcErrorCounter);
}

void RaftMetrics::reportGrpcError(int errorCode) {
  auto grpcMetricsFamily = MetricsCollector::Instance()
    .findMetric<prometheus::Family<prometheus::Counter>>(RaftMetrics::kGrpcErrorCounterName);
  auto & grpcMetrics = grpcMetricsFamily
    ->Add({{"error_code", std::to_string(errorCode)}});
  grpcMetrics.Increment();
}

}  /// namespace gringofts

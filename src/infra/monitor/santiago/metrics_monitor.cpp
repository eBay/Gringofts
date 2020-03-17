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

#include <memory>
#include <string>

#include "metrics_monitor.h"
#include "metrics_collector.h"

namespace santiago {

const char MetricsMonitor::MetricAddress[] = "0.0.0.0";
const char MetricsMonitor::MetricEndpoint[] = "/metrics";

MetricsMonitor::MetricsMonitor(uint16_t port)
        : initDone(false),
          exposer(std::make_shared<prometheus::Exposer>(
                  std::string(MetricAddress) + ":" + std::to_string(port),
                  MetricEndpoint)) {
}

MetricsMonitor &MetricsMonitor::Instance(uint16_t port) {
    static MetricsMonitor instance{port};
    return instance;
}

bool MetricsMonitor::init() {
    if (!initDone) {
        auto &metricsCollector = MetricsCollector::Instance();
        metricsCollector.init();
        exposer->RegisterCollectable(metricsCollector.GetRegistry());
        initDone = true;
    }

    return true;
}

bool MetricsMonitor::shutdown() {
    if (initDone) {
        MetricsCollector::Instance().shutdown();
    }

    return true;
}

MetricsMonitor::~MetricsMonitor() {
}

}  /// namespace santiago

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

#ifndef SRC_INFRA_MONITOR_SANTIAGO_METRICS_MONITOR_H_
#define SRC_INFRA_MONITOR_SANTIAGO_METRICS_MONITOR_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <prometheus/exposer.h>

namespace santiago {

class MetricsMonitor {
 private:
    explicit MetricsMonitor(uint16_t port);

 public:
    ~MetricsMonitor();

    static const char MetricAddress[];
    static const char MetricEndpoint[];

    // return a singleton instance of this class
    static MetricsMonitor& Instance(uint16_t port = 9091);

    bool init();
    bool shutdown();
    void registry(std::shared_ptr<prometheus::Registry> reg) {
      exposer->RegisterCollectable(reg);
    }

 private:
    bool initDone;
    std::shared_ptr<prometheus::Exposer> exposer;
};

}  /// namespace santiago

#endif  // SRC_INFRA_MONITOR_SANTIAGO_METRICS_MONITOR_H_

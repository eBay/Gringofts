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

#include "metrics_collector.h"
#include "metrics_category.h"

namespace santiago {

MetricsCollector::MetricsCollector()
        : registry(std::make_shared<prometheus::Registry>()) {
}

MetricsCollector &MetricsCollector::Instance() {
    static MetricsCollector instance;
    return instance;
}

MetricsCollector::~MetricsCollector() {
    metricsHolder.clear();
}

bool MetricsCollector::shutdown() {
    metricsHolder.clear();

    for (MetricsCategory* cat : metricsCategories) {
        delete cat;
    }
    metricsCategories.clear();

    return true;
}

bool MetricsCollector::init() {
    return true;
}

}  /// namespace santiago

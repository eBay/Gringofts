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

#ifndef SRC_INFRA_MONITOR_MONITORTYPES_H_
#define SRC_INFRA_MONITOR_MONITORTYPES_H_

#include "santiago/AppInfo.h"

#include "../../infra/util/Util.h"

namespace gringofts {

const prometheus::Histogram::BucketBoundaries kBucketBoundaries = prometheus::Histogram::BucketBoundaries {
    0,
    1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
    10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0,
    100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0,
    1000.0, 2000.0, 4000.0, 6000.0, 8000.0,
    10000.0, 20000.0, 40000.0, 60000.0, 80000.0,
    100000.0
};

inline santiago::AppInfo& getAppInfoMetricsCenter() {
  return gringofts::Singleton<santiago::AppInfo>::getInstance();
}

inline santiago::MetricsCenter::SummaryType getSummary(const std::string &name,
                                                       const std::map<std::string, std::string> &labels) {
  return getAppInfoMetricsCenter().summary(name, labels);
}

inline santiago::MetricsCenter::HistogramType getHistogram(const std::string &name,
                                                      const std::map<std::string, std::string> &labels) {
  return getAppInfoMetricsCenter().histogram(name, labels, kBucketBoundaries);
}

inline santiago::MetricsCenter::HistogramType getHistogram(const std::string &name,
                                                      const std::map<std::string, std::string> &labels,
                                                      const prometheus::Histogram::BucketBoundaries &bucketBoundaries) {
  return getAppInfoMetricsCenter().histogram(name, labels, bucketBoundaries);
}

inline santiago::MetricsCenter::CounterType getCounter(const std::string &name,
                                                       const std::map<std::string, std::string> &labels) {
  return getAppInfoMetricsCenter().counter(name, labels);
}

inline santiago::MetricsCenter::GaugeType getGauge(const std::string &name,
                                                   const std::map<std::string, std::string> &labels) {
  return getAppInfoMetricsCenter().gauge(name, labels);
}

}  /// namespace gringofts

#endif  // SRC_INFRA_MONITOR_MONITORTYPES_H_


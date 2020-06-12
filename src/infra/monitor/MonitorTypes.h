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
#include "santiago/Server.h"

#include "../../infra/util/Util.h"

namespace gringofts {

template<class... Arg>
inline santiago::Server& getMonitorServer(Arg &&... args) {
  return gringofts::Singleton<santiago::Server>::getInstance(std::forward<Arg>(args)...);
}

inline santiago::AppInfo& getAppInfoMetricsCenter() {
  return gringofts::Singleton<santiago::AppInfo>::getInstance();
}

inline santiago::MetricsCenter::SummaryType getSummary(const std::string &name,
                                                       const std::map<std::string, std::string> &labels) {
  return getAppInfoMetricsCenter().summary(name, labels);
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


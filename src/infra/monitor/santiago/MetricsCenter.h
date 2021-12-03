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

#ifndef SRC_INFRA_MONITOR_SANTIAGO_METRICSCENTER_H_
#define SRC_INFRA_MONITOR_SANTIAGO_METRICSCENTER_H_

#include "prometheus/PrometheusMetrics.h"

#include "Metrics.h"

namespace prometheus {
class Registry;
}

namespace santiago {

template<class T>
class PrometheusMetricsFactory;
class PrometheusCounter;
class MetricsCenter {
 public:
  typedef std::map<std::string, std::string> LabelType;
  typedef Counter<PrometheusCounter> CounterType;
  typedef Gauge<PrometheusGauge> GaugeType;
  typedef Summary<PrometheusSummary> SummaryType;
  typedef PrometheusMetricsFactory<CounterType> CounterFactory;
  typedef PrometheusMetricsFactory<GaugeType> GaugeFactory;
  typedef PrometheusMetricsFactory<SummaryType> SummaryFactory;
  MetricsCenter();
  virtual ~MetricsCenter() = default;
  CounterType counter(const std::string &name, const LabelType &label, const std::string &help = "");
  GaugeType gauge(const std::string &name, const LabelType &label, const std::string &help = "");
  SummaryType summary(const std::string &name, const LabelType &label, const std::string &help = "");
  std::shared_ptr<prometheus::Registry> getRegistryPtr() { return mRegistryPtr; }
 private:
  std::shared_ptr<prometheus::Registry> mRegistryPtr;
  std::shared_ptr<CounterFactory> mCouterFactory;
  std::shared_ptr<GaugeFactory> mGaugeFactory;
  std::shared_ptr<SummaryFactory> mSummaryFactory;
};

}  /// namespace santiago

#endif  // SRC_INFRA_MONITOR_SANTIAGO_METRICSCENTER_H_

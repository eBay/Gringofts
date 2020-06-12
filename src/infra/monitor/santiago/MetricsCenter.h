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

namespace santiago {

const ::prometheus::Summary::Quantiles SummaryQuantiles = ::prometheus::Summary::Quantiles{
    {0.99, 0.001},
    {0.995, 0.0005},
    {0.999, 0.0001},
};

class MetricsCenter {
 public:
  typedef std::map<std::string, std::string> LabelType;
  typedef Counter<santiago::prometheus::Counter> CounterType;
  typedef Gauge<santiago::prometheus::Gauge> GaugeType;
  typedef Summary<santiago::prometheus::Summary> SummaryType;
  typedef santiago::prometheus::MetricsFactory<CounterType> CounterFactory;
  typedef santiago::prometheus::MetricsFactory<GaugeType> GaugeFactory;
  typedef santiago::prometheus::MetricsFactory<SummaryType> SummaryFactory;
  MetricsCenter() : mRegistryPtr(std::make_shared<::prometheus::Registry>()),
                    mCounterFactory(std::make_shared<CounterFactory>(mRegistryPtr)),
                    mGaugeFactory(std::make_shared<GaugeFactory>(mRegistryPtr)),
                    mSummaryFactory(std::make_shared<SummaryFactory>(mRegistryPtr)) {}

  virtual ~MetricsCenter() = default;
  CounterType counter(const std::string &name, const LabelType &label, const std::string &help = "") {
    return mCounterFactory->get(name, label, help);
  }
  GaugeType gauge(const std::string &name, const LabelType &label, const std::string &help = "") {
    return mGaugeFactory->get(name, label, help);
  }
  SummaryType summary(const std::string &name, const LabelType &label, const std::string &help = "") {
    return mSummaryFactory->get(name, label, help, SummaryQuantiles);
  }
  std::shared_ptr<::prometheus::Registry> getRegistryPtr() const { return mRegistryPtr; }

 private:
  std::shared_ptr<::prometheus::Registry> mRegistryPtr;
  std::shared_ptr<CounterFactory> mCounterFactory;
  std::shared_ptr<GaugeFactory> mGaugeFactory;
  std::shared_ptr<SummaryFactory> mSummaryFactory;
};

}  /// namespace santiago

#endif  // SRC_INFRA_MONITOR_SANTIAGO_METRICSCENTER_H_

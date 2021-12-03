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

#include "MetricsCenter.h"
#include "prometheus/summary.h"

namespace santiago {

const prometheus::Summary::Quantiles SummaryQuantiles = prometheus::Summary::Quantiles{
    {0.99, 0.001},
    {0.995, 0.0005},
    {0.999, 0.0001}};

MetricsCenter::MetricsCenter()
    : mRegistryPtr(std::make_shared<prometheus::Registry>()),
      mCouterFactory(std::make_shared<CounterFactory>(*mRegistryPtr)),
      mGaugeFactory(std::make_shared<GaugeFactory>(*mRegistryPtr)),
      mSummaryFactory(std::make_shared<SummaryFactory>(*mRegistryPtr)) {
}

MetricsCenter::CounterType MetricsCenter::counter(const std::string &name,
                                                  const LabelType &label,
                                                  const std::string &help) {
  return mCouterFactory->get(name, label, help);
}

MetricsCenter::GaugeType MetricsCenter::gauge(const std::string &name,
                                              const santiago::MetricsCenter::LabelType &label,
                                              const std::string &help) {
  return mGaugeFactory->get(name, label, help);
}

MetricsCenter::SummaryType MetricsCenter::summary(const std::string &name,
                                                  const santiago::MetricsCenter::LabelType &label,
                                                  const std::string &help) {
  return mSummaryFactory->get(name, label, help, SummaryQuantiles);
}

}  /// namespace santiago

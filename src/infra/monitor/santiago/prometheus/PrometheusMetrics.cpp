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

#include "PrometheusMetrics.h"


namespace santiago {

PrometheusCounter::PrometheusCounter(prometheus::Counter &prometheusCounter) :
    mCounter(prometheusCounter) {}

void PrometheusCounter::increase() {
  mCounter.Increment();
}

void PrometheusCounter::increase(double val) {
  mCounter.Increment(val);
}

double PrometheusCounter::value() {
  return mCounter.Value();
}

PrometheusGauge::PrometheusGauge(prometheus::Gauge &prometheusGauge) :
    mGauge(prometheusGauge) {}

double PrometheusGauge::value() {
  return mGauge.Value();
}

void PrometheusGauge::set(double val) {
  mGauge.Set(val);
}

PrometheusSummary::PrometheusSummary(prometheus::Summary &prometheusSummary) :
    mSummary(prometheusSummary) {}

void PrometheusSummary::observe(double val) {
  mSummary.Observe(val);
}

}  /// namespace santiago

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

#include "Metrics.h"

#include "prometheus/PrometheusMetrics.h"

namespace santiago {

template<>
void Counter<PrometheusCounter>::increase() {
  return mImplPtr->increase();
}

template<>
void Counter<PrometheusCounter>::increase(double val) {
  return mImplPtr->increase(val);
}

template<>
double Counter<PrometheusCounter>::value() {
  return mImplPtr->value();
}

template<>
void Gauge<PrometheusGauge>::set(double value) {
  return mImplPtr->set(value);
}

template<>
double Gauge<PrometheusGauge>::value() {
  return mImplPtr->value();
}


template <>
void Summary<PrometheusSummary>::observe(double val) {
  mImplPtr->observe(val);
}

}  /// namespace santiago

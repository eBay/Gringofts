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

#ifndef SRC_INFRA_MONITOR_SANTIAGO_PROMETHEUS_PROMETHEUSMETRICS_H_
#define SRC_INFRA_MONITOR_SANTIAGO_PROMETHEUS_PROMETHEUSMETRICS_H_

#include <unordered_map>

#include "prometheus/counter.h"
#include "prometheus/gauge.h"
#include "prometheus/registry.h"
#include "prometheus/summary.h"

#include "../Metrics.h"

namespace santiago {

class PrometheusCounter {
 public:
  typedef prometheus::Counter InnerType;
  typedef prometheus::detail::Builder<prometheus::Counter> InnerBuilder;
  explicit PrometheusCounter(InnerType &);
  PrometheusCounter(const PrometheusCounter &) = default;
  PrometheusCounter(PrometheusCounter &&) = default;
  void increase();
  void increase(double);
  double value();
 private:
  InnerType &mCounter;
};

class PrometheusGauge {
 public:
  typedef prometheus::Gauge InnerType;
  typedef prometheus::detail::Builder<prometheus::Gauge> InnerBuilder;
  explicit PrometheusGauge(InnerType &);
  PrometheusGauge(const PrometheusGauge &) = default;
  PrometheusGauge(PrometheusGauge &&) = default;
  void set(double);
  double value();
 private:
  InnerType &mGauge;
};

class PrometheusSummary {
 public:
  typedef prometheus::Summary InnerType;
  typedef prometheus::detail::Builder<prometheus::Summary> InnerBuilder;
  explicit PrometheusSummary(InnerType &);
  PrometheusSummary(const PrometheusSummary &) = default;
  PrometheusSummary(PrometheusSummary &&) = default;
  void observe(double);
 private:
  InnerType &mSummary;
};

template<class T>
class PrometheusMetricsFactory {
 public:
  typedef typename T::ImplType::InnerType InnerMetricsType;
  typedef typename T::ImplType::InnerBuilder InnerMetricsBuilder;
  typedef prometheus::Family<InnerMetricsType> MetricsFamily;
  explicit PrometheusMetricsFactory(prometheus::Registry &registry) : mRegistry(registry) {}
  template<class ... ArgT>
  T get(const std::string &name,
        const std::map<std::string, std::string> &labels,
        const std::string &help,
        ArgT &&...args) {
    std::lock_guard lk{mMutex};
    if (mFamilies.count(name) == 0) {
      InnerMetricsBuilder builder{};
      auto &family = builder.Name(name).Help(help).Register(mRegistry);
      mFamilies[name] = &family;
    }
    return T(mFamilies[name]->Add(labels, std::forward<ArgT>(args)...));
  }

 private:
  std::unordered_map<std::string, MetricsFamily *> mFamilies;
  prometheus::Registry &mRegistry;
  std::mutex mMutex;
};

}  /// namespace santiago

#endif  //  SRC_INFRA_MONITOR_SANTIAGO_PROMETHEUS_PROMETHEUSMETRICS_H_

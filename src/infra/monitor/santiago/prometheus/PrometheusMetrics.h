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

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/summary.h>
#include <prometheus/registry.h>

#include "../Metrics.h"

namespace santiago::prometheus {

template<class T, class B>
class MetricHolder {
 public:
  typedef T InnerType;
  typedef B InnerBuilder;
  explicit MetricHolder(T &ref) : mReference(ref) {}
  MetricHolder(const MetricHolder &) = default;
  T *operator->() const { return &mReference; }
  T *operator->() { return &mReference; }
 private:
  T &mReference;
};

typedef MetricHolder<::prometheus::Counter, ::prometheus::detail::Builder<::prometheus::Counter>> Counter;
typedef MetricHolder<::prometheus::Gauge, ::prometheus::detail::Builder<::prometheus::Gauge>> Gauge;
typedef MetricHolder<::prometheus::Summary, ::prometheus::detail::Builder<::prometheus::Summary>> Summary;

template<class T>
class MetricsFactory {
 public:
  typedef typename T::ImplType::InnerType InnerMetricsType;
  typedef typename T::ImplType::InnerBuilder InnerMetricsBuilder;
  typedef ::prometheus::Family<InnerMetricsType> MetricsFamily;
  explicit MetricsFactory(std::shared_ptr<::prometheus::Registry> registry) : mRegistry(registry) {}
  template<class ... ArgT>
  T get(const std::string &name,
        const std::map<std::string, std::string> labels,
        const std::string &help,
        ArgT &&...args) {
    if (mFamilies.count(name) == 0) {
      InnerMetricsBuilder builder{};
      auto &family = builder.Name(name).Help(help).Register(*mRegistry);
      mFamilies[name] = &family;
    }
    return T(mFamilies[name]->Add(labels, std::forward<ArgT>(args)...));
  }

 private:
  std::unordered_map<std::string, MetricsFamily *> mFamilies;
  std::shared_ptr<::prometheus::Registry> mRegistry;
};

}  /// namespace santiago::prometheus

#endif  //  SRC_INFRA_MONITOR_SANTIAGO_PROMETHEUS_PROMETHEUSMETRICS_H_

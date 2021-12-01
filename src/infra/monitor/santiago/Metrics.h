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

#ifndef SRC_INFRA_MONITOR_SANTIAGO_METRICS_H_
#define SRC_INFRA_MONITOR_SANTIAGO_METRICS_H_

#include <map>
#include <memory>

namespace santiago {

template<class Impl>
class Counter {
 public:
  typedef Impl ImplType;
  template<class ... ArgT>
  explicit Counter(ArgT &&...args):
      mImplPtr(std::make_shared<ImplType>(std::forward<ArgT>(args)...)) {}
  Counter(const Counter &_c) : mImplPtr(_c.mImplPtr) {}
  Counter(Counter &&_c) : mImplPtr(_c.mImplPtr) {}
  void increase();
  void increase(double);
  double value();
 private:
  std::shared_ptr<ImplType> mImplPtr;
};

template<class Impl>
class Gauge {
 public:
  typedef Impl ImplType;
  template<class ... ArgT>
  explicit Gauge(ArgT &&...args):
      mImplPtr(std::make_shared<ImplType>(std::forward<ArgT>(args)...)) {}
  Gauge(const Gauge &_c) : mImplPtr(_c.mImplPtr) {}
  void set(double value);
  double value();
 private:
  std::shared_ptr<ImplType> mImplPtr;
};

template<class Impl>
class Summary {
 public:
  typedef Impl ImplType;
  template<class ... ArgT>
  explicit Summary(ArgT &&...args):
      mImplPtr(std::make_shared<ImplType>(std::forward<ArgT>(args)...)) {}
  Summary(const Summary &_c) : mImplPtr(_c.mImplPtr) {}
  void observe(double);
 private:
  std::shared_ptr<ImplType> mImplPtr;
};

}  /// namespace santiago

#endif  // SRC_INFRA_MONITOR_SANTIAGO_METRICS_H_

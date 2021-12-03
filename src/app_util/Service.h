/************************************************************************
Copyright 2019-2021 eBay Inc.
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

#ifndef SRC_APP_UTIL_SERVICE_H_
#define SRC_APP_UTIL_SERVICE_H_
#include <type_traits>
#include <thread>

namespace gringofts::app {
class Service {
 public:
  Service() = default;
  Service(const Service &) = delete;

  virtual void start() = 0;
  virtual void stop() = 0;
  virtual ~Service() = default;
};

template<typename T, typename = void>
struct is_runnable : std::false_type {};

template<typename T>
struct is_runnable<T, std::void_t<decltype(std::declval<T>().run()),
                                  decltype(std::declval<T>().shutdown())>> : std::true_type {
};

template<typename T, std::enable_if_t<is_runnable<T>::value, bool> = true>
class RunnableService : public Service {
 public:
  template<class ...Args>
  RunnableService(std::string serviceName, Args ...args)
      :mServiceName(std::move(serviceName)), mImpl(std::forward<Args>(args)...) {}

  void start() override {
    mThread = std::thread([this]() {
      pthread_setname_np(pthread_self(), "Service");
      mImpl.run();
    });
  }

  void stop() override {
    mImpl.shutdown();
    if (mThread.joinable()) {
      mThread.join();
    }
  }

 private:
  std::string mServiceName;
  T mImpl;
  std::thread mThread;
};

}  // namespace gringofts::app
#endif  // SRC_APP_UTIL_SERVICE_H_

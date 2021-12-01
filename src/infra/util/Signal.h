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

#ifndef SRC_INFRA_UTIL_SIGNAL_H_
#define SRC_INFRA_UTIL_SIGNAL_H_

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <unordered_map>

#include "../mpscqueue/MpscQueue.h"
#include "Util.h"

namespace gringofts {
class SignalSlot;
class Signal {
 public:
  typedef uint64_t Type;
  typedef std::function<void(const Signal &)> Handler;
  typedef std::shared_ptr<Signal> Ptr;
  virtual ~Signal() = default;
  static SignalSlot hub;
};

template<typename T>
class FutureSignal : public Signal {
 public:
  void passValue(T t) const {
    mPromise.set_value(std::move(t));
  }
  std::future<T> getFuture() const {
    return mPromise.get_future();
  }
 private:
  mutable std::promise<T> mPromise;
};

class SignalSlot {
 public:
  SignalSlot();
  SignalSlot(SignalSlot const &) = delete;
  ~SignalSlot() { stop(); }

  template<typename T>
  inline void handle(Signal::Handler handler) {
    registerEventHandler(&typeid(T), std::move(handler));
  }
  inline void operator<<(Signal::Ptr ptr) {
    dispatch(std::move(ptr));
  }

 private:
  void dispatch(Signal::Ptr);
  void registerEventHandler(const std::type_info *, Signal::Handler);
  void run();
  void stop();
  std::unordered_map<const std::type_info *, Signal::Handler> mHandlerTable;
  std::unique_ptr<gringofts::MpscQueue<Signal::Ptr>> mEventQueue;
  std::thread mThread;
  std::atomic_bool mRunning;
};

}  /// namespace gringofts
#endif  // SRC_INFRA_UTIL_SIGNAL_H_

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

#include "Signal.h"

#include <utility>
#include "../common_types.h"
#include "../mpscqueue/MpscDoubleBufferQueue.h"

namespace gringofts {

SignalSlot Signal::hub;

class EndSignal : public Signal {};

SignalSlot::SignalSlot() :
    mEventQueue(std::make_unique<MpscDoubleBufferQueue<Signal::Ptr>>()),
    mRunning(true) {
  handle<EndSignal>([this](const Signal &) {
    SPDLOG_INFO("stop event dispatcher");
    mRunning = false;
  });
  mThread = std::thread([this]() {
    SPDLOG_INFO("start event dispatcher");
    run();
  });
}

void SignalSlot::registerEventHandler(const std::type_info *type, Signal::Handler handler) {
  mHandlerTable[type] = std::move(handler);
}
void SignalSlot::dispatch(Signal::Ptr signal) {
  mEventQueue->enqueue(signal);
}

void SignalSlot::run() {
  pthread_setname_np(pthread_self(), "signal_dispatch");
  while (mRunning) {
    auto signalPtr = mEventQueue->dequeue();
    auto &signal = *signalPtr;
    const auto *type = &typeid(signal);
    auto it = mHandlerTable.find(type);
    if (it != mHandlerTable.end()) {
      it->second(signal);
    } else {
      SPDLOG_WARN("no matched handler for type {}", type->name());
    }
  }
}

void SignalSlot::stop() {
  dispatch(std::make_shared<EndSignal>());
  if (mThread.joinable()) {
    mThread.join();
  }
}
}  /// namespace gringofts

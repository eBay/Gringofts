/************************************************************************
Copyright 2021-2022 eBay Inc.
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

#include <spdlog/spdlog.h>

#include "Loop.h"
namespace goblin::kvengine::execution {

void ThreadLoop::start() {
  mRunning = true;
  mThread = std::make_unique<std::thread>([this]() {
    while (mRunning && loop()) {}
    /// SPDLOG_INFO("finished loop");
  });
}

void ThreadLoop::stop() {
  mRunning = false;
}

void ThreadLoop::join() {
  if (mThread && mThread->joinable()) {
    mThread->join();
  }
}
}  // namespace goblin::kvengine::execution

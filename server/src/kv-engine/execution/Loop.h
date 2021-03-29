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

#ifndef SERVER_SRC_KV_ENGINE_EXECUTION_LOOP_H_
#define SERVER_SRC_KV_ENGINE_EXECUTION_LOOP_H_

#include <atomic>
#include <memory>
#include <thread>

namespace goblin::kvengine::execution {
class Loop {
 public:
  virtual ~Loop() = default;
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void join() = 0;
  void waitForStop() {
    stop();
    join();
  }
};

class ThreadLoop : public Loop {
 public:
  ThreadLoop() : mRunning(false) {}
  // no copy and move
  explicit ThreadLoop(const Loop &) = delete;
  explicit ThreadLoop(Loop &&) = delete;
  virtual ~ThreadLoop() = default;

  void start() override;
  void stop() override;
  void join() override;
  virtual bool loop() = 0;

 private:
  std::atomic_bool mRunning;
  std::unique_ptr<std::thread> mThread;
};
}  // namespace goblin::kvengine::execution
#endif  // SERVER_SRC_KV_ENGINE_EXECUTION_LOOP_H_

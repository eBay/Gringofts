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

#ifndef SERVER_SRC_KV_ENGINE_EXECUTION_EXECUTIONSERVICEIMPL_H_
#define SERVER_SRC_KV_ENGINE_EXECUTION_EXECUTIONSERVICEIMPL_H_
#include <vector>
#include <memory>
#include <mutex>

#include "ExecutionService.h"
#include "QueueWorker.h"

namespace goblin::kvengine::execution {

template<typename T>
class ExecutionServiceImpl : public ExecutionService<T> {
 public:
  typedef QueueWorker<T> Worker;
  typedef std::unique_ptr<Worker> WorkerPtr;
  ExecutionServiceImpl(typename Worker::ProcessorPtr processor, int size):
    mProcessor(processor), mPointer(0) {
      mWorkers.reserve(size);
      for (int i = 0; i < size; ++i) {
        mWorkers.push_back(std::make_unique<Worker>(mProcessor));
      }
  }
  ExecutionServiceImpl(typename FuncProcessor<T>::Func func, int size):
    ExecutionServiceImpl(std::make_shared<FuncProcessor<T>>(func), size) {
  }

  void start() override {
    for (auto &worker : mWorkers) {
      worker->start();
    }
  }
  void shutdown() override {
    SPDLOG_INFO("shutdown worker");
    for (auto &worker : mWorkers) {
      worker->stop();
    }
    for (auto &worker : mWorkers) {
      worker->join();
    }
  }

  void submit(const T &input) override {
    Worker *worker;
    {
      std::unique_lock lock(mMutex);
      worker = mWorkers[mPointer].get();
      mPointer = (mPointer + 1) % mWorkers.size();
    }
    if (worker) {
      worker->submit(input);
    }
  }

 private:
  typename Worker::ProcessorPtr mProcessor;
  std::vector<WorkerPtr> mWorkers;
  int mPointer;
  std::mutex mMutex;
};
}  // namespace goblin::kvengine::execution
#endif  // SERVER_SRC_KV_ENGINE_EXECUTION_EXECUTIONSERVICEIMPL_H_

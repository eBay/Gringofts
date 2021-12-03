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

#ifndef SRC_INFRA_MPSCQUEUE_MPSCDOUBLEBUFFERQUEUE_H_
#define SRC_INFRA_MPSCQUEUE_MPSCDOUBLEBUFFERQUEUE_H_

#include <atomic>
#include <list>
#include <memory>
#include <mutex>

#include <spdlog/spdlog.h>

#include "MpscQueue.h"

namespace gringofts {

/**
 * A multi-producer single-consumer thread-safe queue.
 *
 * Internally it uses two queues to reduce contention.
 * One for producer and the other for consumer.
 * Queues are flipped when current consumer queue is empty.
 * The typical use scenario is fast producer slow consumer.
 */
template<typename T>
class MpscDoubleBufferQueue final : public MpscQueue<T> {
 public:
  MpscDoubleBufferQueue() {
    mProducerQueue = std::make_unique<Queue>();
    mConsumerQueue = std::make_unique<Queue>();
  }

  ~MpscDoubleBufferQueue() override = default;

  void enqueue(const T &) override;
  const T dequeue() override;

  uint64_t size() const override { return mConsumerQueue->size(); }
  uint64_t estimateTotalSize() const override {
    return mConsumerQueue->size() + mProducerQueue->size();
  }
  bool empty() const override { return mQueueSize == 0; }

  void shutdown() override {
    mShouldExit = true;
    mCondVar.notify_one();
    SPDLOG_INFO("Stop queue");
  }

 private:
  using Queue = std::list<T>;

  /// queue that producer writes to
  std::unique_ptr<Queue> mProducerQueue;
  /// queue that consumer reads from
  std::unique_ptr<Queue> mConsumerQueue;

  /// Mutex for \p mProducerQueue
  std::mutex mMutex;
  /// CV for flipping \p mProducerQueue and \p mConsumerQueue.
  std::condition_variable mCondVar;

  /// Queue Size for both \p mProducerQueue and \p mConsumerQueue.
  std::atomic<uint64_t> mQueueSize = 0;

  /// true if the \p mProducerQueue should
  /// stop accepting new requests from producer.
  std::atomic<bool> mShouldExit = false;
};

}  /// namespace gringofts

#include "MpscDoubleBufferQueue.cpp"

#endif  // SRC_INFRA_MPSCQUEUE_MPSCDOUBLEBUFFERQUEUE_H_

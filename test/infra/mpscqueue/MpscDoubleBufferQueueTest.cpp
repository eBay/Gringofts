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

#include <atomic>

#include <gtest/gtest.h>

#include "../../../src/infra/common_types.h"

namespace gringofts::test {

class MpscDoubleBufferQueueTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    mSpscQueue = std::make_unique<MpscDoubleBufferQueue<int>>();
  }

  virtual void TearDown() {
  }

  std::unique_ptr<MpscQueue<int>> mSpscQueue;
};

TEST_F(MpscDoubleBufferQueueTest, DequeueReturnValueIfEnqueued) {
  // init
  int expected1 = 1, expected2 = 2;

  // behavior
  mSpscQueue->enqueue(expected1);
  mSpscQueue->enqueue(expected2);
  const auto actual1 = mSpscQueue->dequeue();
  const auto actual2 = mSpscQueue->dequeue();

  // assert
  EXPECT_EQ(mSpscQueue->size(), 0);
  EXPECT_EQ(mSpscQueue->empty(), true);
  EXPECT_EQ(actual1, expected1);
  EXPECT_EQ(actual2, expected2);
}

TEST_F(MpscDoubleBufferQueueTest, DequeuIsBlocked) {
  // init
  int expected1 = 1, expected2 = 2;
  std::atomic<int> id(0);
  int producerId = 0, consumerId = 0;
  int actual1 = 0, actual2 = 0;

  // behavior
  auto consumerThread = std::thread([this, &id, &actual1, &actual2, &consumerId]() {
    actual1 = mSpscQueue->dequeue();
    actual2 = mSpscQueue->dequeue();
    consumerId = id++;
  });
  auto producerThread = std::thread([this, &id, &expected1, &expected2, &producerId] {
    mSpscQueue->enqueue(expected1);
    producerId = id++;
    mSpscQueue->enqueue(expected2);
  });

  producerThread.join();
  consumerThread.join();

  // assert
  EXPECT_TRUE(mSpscQueue->empty());
  EXPECT_EQ(mSpscQueue->size(), 0);
  EXPECT_EQ(actual1, expected1);
  EXPECT_EQ(actual2, expected2);
  EXPECT_EQ(producerId, 0);
  EXPECT_EQ(consumerId, 1);
}

TEST_F(MpscDoubleBufferQueueTest, EnqueueAfterShutdownWillThrowException) {
  // init
  mSpscQueue->enqueue(1);

  // behavior
  mSpscQueue->shutdown();

  // assert
  EXPECT_THROW(mSpscQueue->enqueue(2), QueueStoppedException);
}

TEST_F(MpscDoubleBufferQueueTest, DequeueWillNotBlockAfterShutdownIfEmptyQueue) {
  // behavior
  mSpscQueue->shutdown();

  // assert
  EXPECT_THROW(mSpscQueue->dequeue(), QueueStoppedException);  // throw when the consumer queue is empty
}

TEST_F(MpscDoubleBufferQueueTest, DequeueAfterShutdownWillConsumeAllBeforeThrowException) {
  // init
  mSpscQueue->enqueue(1);
  mSpscQueue->enqueue(2);

  // behavior
  mSpscQueue->shutdown();

  // assert
  EXPECT_NO_THROW(mSpscQueue->dequeue());  // no exception when there is entries in the consumer queue
  EXPECT_NO_THROW(mSpscQueue->dequeue());  // no exception when there is entries in the consumer queue
  EXPECT_THROW(mSpscQueue->dequeue(), QueueStoppedException);  // throw when the consumer queue is empty
}

TEST_F(MpscDoubleBufferQueueTest, DequeueWillThrowWhenShutdownHappens) {
  // behavior
  auto consumerThread = std::thread([this]() {
    // sleep to ensure dequeue will be called first before shutdown
    sleep(1);
    mSpscQueue->shutdown();
  });

  // assert
  EXPECT_THROW(mSpscQueue->dequeue(), QueueStoppedException);  // throw when the consumer queue is empty
  consumerThread.join();
}

}  /// namespace gringofts::test

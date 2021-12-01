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

#ifndef SRC_INFRA_MPSCQUEUE_MPSCQUEUE_H_
#define SRC_INFRA_MPSCQUEUE_MPSCQUEUE_H_

#include <cstdint>
#include <exception>

namespace gringofts {

class QueueStoppedException : public ::std::exception {
 public:
  const char *what() const throw() {
    return "Queue has stopped.";
  }
};

/**
 * A multi-producer single-consumer queue
 * TODO: need some code refacotr later.
 */
template<typename T>
class MpscQueue {
 public:
  virtual ~MpscQueue() = default;

  /**
   * Append an item to the tail of the queue.
   * This method will block until the item has been successfully inserted.
   * @throw ::gringofts::QueueStoppedException if queue has been shut down
   */
  virtual void enqueue(const T &) = 0;

  /**
   * Remove and return the item at the head of the queue.
   * This method will block until the queue is not empty.
   * @return the item at the head of the queue
   * @throw ::gringofts::QueueStoppedException if queue has been shut down
   */
  virtual const T dequeue() = 0;

  /**
   * Return the total count of items in the consumer queue.
   * @return the total count of items in the consumer queue
   */
  virtual uint64_t size() const = 0;

  /**
   * Return the total count of items in both consumer and producer queue.
   * It might not be correct due to concurrent access
   * @return the total count of items in both consumer and producer queue
   */
  virtual uint64_t estimateTotalSize() const = 0;

  /**
   * Return true, if queue is empty.
   */
  virtual bool empty() const = 0;

  /**
   * Stop accepting new requests from producer.
   * Entries already in the queue will be all consumed.
   */
  virtual void shutdown() = 0;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_MPSCQUEUE_MPSCQUEUE_H_

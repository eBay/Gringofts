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

namespace gringofts {

template<typename T>
void MpscDoubleBufferQueue<T>::enqueue(const T &t) {
  if (mShouldExit) {
    throw QueueStoppedException();
  }

  std::unique_lock lock(mMutex);

  bool isEmpty = mProducerQueue->empty();
  mProducerQueue->push_back(t);

  ++mQueueSize;

  if (isEmpty) {
    mCondVar.notify_one();
  }
}

template<typename T>
const T MpscDoubleBufferQueue<T>::dequeue() {
  if (mConsumerQueue->empty()) {
    std::unique_lock lock(mMutex);

    if (mProducerQueue->empty()) {
      /// this call should not block
      /// if it's empty and the shouldExit signal is fired before
      if (mShouldExit) {
        throw QueueStoppedException();
      }

      mCondVar.wait(lock, [this] { return !mProducerQueue->empty() || mShouldExit; });

      /**
       * after wakeup, it will be following three case:
       *   1) mProducerQueue not empty, mShouldExit is true
       *   2) mProducerQueue not empty, mShouldExit is false
       *   3) mProducerQueue empty, mShouldExit is true
       *
       * attention that, for case 1, we need flip mProducerQueue and mConsumerQueue
       * to handle the last left requests in producer queue.
       */
      if (mProducerQueue->empty()) {
        throw QueueStoppedException();
      }
    }

    std::swap(mProducerQueue, mConsumerQueue);
  }

  --mQueueSize;

  const T t = mConsumerQueue->front();
  mConsumerQueue->pop_front();

  return t;
}

}  /// namespace gringofts

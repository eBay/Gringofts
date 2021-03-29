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
#ifndef SERVER_SRC_KV_ENGINE_STRATEGY_CACHEEVICTION_H_
#define SERVER_SRC_KV_ENGINE_STRATEGY_CACHEEVICTION_H_

#include <list>
#include <variant>

#include "../execution/ExecutionServiceImpl.h"
#include "../execution/EventApplyLoop.h"
#include "../store/KVStore.h"

namespace model {
class Command;
}

namespace goblin::kvengine::strategy {
using KVStrategy = store::KVObserver;

class CacheEviction : public KVStrategy {
 public:
  void onWriteValue(const store::KeyType &key,
      const store::ValueType &value,
      const store::VersionType &version) override {
    onWriteValue(key, value.size());
  }
  void onDeleteKey(
      const store::KeyType &key,
      const store::VersionType &version,
      const store::VersionType &deleteRecordVersion) override {
    assert(0);
  }
  virtual void onWriteValue(const store::KeyType &key, size_t valueSize) = 0;
  void onEvictKeys(const std::set<store::KeyType> &keys) override {
     /// we don't do anthing for evict cb
  }
};

enum class KVObserveType {
  READ,
  WRITE,
  EVICT
};

template<typename T>
class CacheEvictionProxy : public CacheEviction {
 public:
  // read : key
  // write : key, value size
  // evict: pointer of batch of key has been evicted
  typedef store::KeyType ReadObsContent;
  typedef std::pair<store::KeyType, size_t> WriteObsContent;
  typedef std::set<store::KeyType> EvictObsContent;
  typedef std::pair<KVObserveType, std::variant<ReadObsContent, WriteObsContent, EvictObsContent>> KVObserveEvent;
  static constexpr int kSingleThread = 1;

  template<typename ...Types>
  explicit CacheEvictionProxy(Types &&...args)
      : mImpl(std::forward<Types>(args)...),
        mExecutionService(
            [this](const KVObserveEvent &event) {
              switch (event.first) {
                case KVObserveType::READ: {
                  auto &key = std::get<ReadObsContent>(event.second);
                  mImpl.onReadValue(key);
                  break;
                }
                case KVObserveType::WRITE: {
                  auto &content = std::get<WriteObsContent>(event.second);
                  mImpl.onWriteValue(content.first, content.second);
                  break;
                }
                case KVObserveType::EVICT: {
                  auto &setPtr = std::get<EvictObsContent>(event.second);
                  mImpl.onEvictKeys(setPtr);
                  break;
                }
              }
            },
            kSingleThread) {
  }
  ~CacheEvictionProxy() { mExecutionService.shutdown(); }
  void onReadValue(const store::KeyType &key) override {
    mExecutionService.submit(KVObserveEvent{KVObserveType::READ, key});
  }
  void onWriteValue(const store::KeyType &key, size_t valueSize) override {
    mExecutionService.submit(KVObserveEvent{KVObserveType::WRITE, std::pair{key, valueSize}});
  }

  void onEvictKeys(const std::set<store::KeyType> &keys) override {
    /// we don't do anthing for evict cb
    /// mExecutionService.submit(KVObserveEvent{KVObserveType::EVICT, keys});
  }
  void start() {
    mExecutionService.start();
  }
  void shutdown() {
    mExecutionService.shutdown();
  }

 private:
  T mImpl;
  execution::ExecutionServiceImpl<KVObserveEvent> mExecutionService;
};

/// this class doesn't guarantee thread safe
/// it needs the CacheEvictionProxy to protect in concurrent context
class LRUEviction : public CacheEviction {
 public:
  typedef std::shared_ptr<model::Command> CommandPtr;
  LRUEviction(
      const execution::EventApplyService &eventApplyLoop,
      execution::ExecutionService<CommandPtr> &commandQueue,  // NOLINT(runtime/references)
      size_t capacity = kDefaultCapacity,
      size_t optimal = kDefaultOptimal,
      size_t minBatch = kDefaultMinBatchToEvict) :
      mEventApplyLoop(eventApplyLoop),
      mCommandQueue(commandQueue),
      mCapacity(capacity),
      mOptimal(optimal),
      mMinBatch(minBatch),
      mCurUsage(0) {
    assert(capacity >= optimal);
  }
  LRUEviction(const LRUEviction &) = delete;
  void onReadValue(const store::KeyType &key) override;
  void onWriteValue(const store::KeyType &key, size_t valueSize) override;

  static constexpr size_t kDefaultCapacity = (size_t) 15 * 1024 * 1024 * 1024;
  static constexpr size_t kDefaultOptimal = (size_t) 10 * 1024 * 1024 * 1024;
  static constexpr size_t kDefaultMinBatchToEvict = (size_t) 100;

 private:
  void maybeSubmitEvictCommand();
  // EAL
  const execution::EventApplyService &mEventApplyLoop;
  // CPL
  execution::ExecutionService<CommandPtr> &mCommandQueue;

  size_t mCapacity;
  size_t mOptimal;
  size_t mMinBatch;
  size_t mCurUsage;
  std::set<store::KeyType> mPendingEvictBatch;

  /// LRU list
  std::list<store::KeyType> mUsed;
  std::unordered_map<store::KeyType, std::pair<size_t, std::list<store::KeyType>::iterator>> mCacheMap;
};
}  // namespace goblin::kvengine::strategy
#endif  // SERVER_SRC_KV_ENGINE_STRATEGY_CACHEEVICTION_H_

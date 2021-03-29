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
#include "CacheEviction.h"

#include <infra/monitor/MonitorTypes.h>

#include "../model/KVEvictCommand.h"

namespace goblin::kvengine::strategy {

void LRUEviction::maybeSubmitEvictCommand() {
  if (mPendingEvictBatch.size() >= mMinBatch) {
    auto version = mEventApplyLoop.getLastAppliedVersion();
    SPDLOG_WARN("submit evict request for {} keys with version {}", mPendingEvictBatch.size(), version);
    auto cmd = std::make_shared<model::KVEvictCommand>(
        mPendingEvictBatch,
        version);
    mCommandQueue.submit(cmd);
    mPendingEvictBatch.clear();
  }
}

void LRUEviction::onReadValue(const store::KeyType &key) {
  if (mCacheMap.find(key) != mCacheMap.end()) {
    auto &pair = mCacheMap[key];
    // update lru list
    mUsed.erase(pair.second);
    mUsed.push_front(key);
    pair.second = mUsed.begin();
  }
}

void LRUEviction::onWriteValue(const store::KeyType &key, size_t valueSize) {
  mCurUsage += valueSize;
  {
    if (mCacheMap.find(key) != mCacheMap.end()) {
      auto &pair = mCacheMap[key];
      // update lru list
      mUsed.erase(pair.second);
      mUsed.push_front(key);
      pair.second = mUsed.begin();
      pair.first = valueSize;
      mCurUsage -= pair.first;
    } else {
      mUsed.push_front(key);
      mCacheMap[key] = {valueSize, mUsed.begin()};
    }
    if (mCurUsage > mOptimal) {
      SPDLOG_WARN("current usage is {} greater than {}", mCurUsage, mOptimal);
      while (mCurUsage > mOptimal && !mUsed.empty()) {
        store::KeyType evictKey = mUsed.back();
        /// we delete item in the LRU list regardless if evict succeed
        /// if evict command failed, we should retry until it succeed
        auto &pair = mCacheMap[evictKey];
        mCacheMap.erase(evictKey);
        mUsed.pop_back();
        mPendingEvictBatch.insert(evictKey);
        mCurUsage -= pair.first;
      }
      SPDLOG_WARN("try to optimise usage to {}", mCurUsage);
      maybeSubmitEvictCommand();
    }
  }
  gringofts::getGauge("cache_memory_usage_in_bytes", {}).set(mCurUsage);
}
}  // namespace goblin::kvengine::strategy

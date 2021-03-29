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


#include "ConcurInMemoryKVStore.h"

#include <spdlog/spdlog.h>

namespace goblin::kvengine::store {

ConcurInMemoryKVStore::ConcurInMemoryKVStore(std::shared_ptr<ReadOnlyKVStore> delegateKVStore) : ProxyKVStore(
    delegateKVStore) {
  for (uint32_t i = 0; i < kConcurLevel; ++i) {
    mSegments[i] = std::make_unique<Segment>(delegateKVStore);
  }
}

utils::Status ConcurInMemoryKVStore::open() {
  auto s = utils::Status::ok();
  for (uint32_t i = 0; i < kConcurLevel; ++i) {
    s = mSegments[i]->mBucketKVStore.open();
    if (!s.isOK()) {
      break;
    }
  }
  return s;
}

utils::Status ConcurInMemoryKVStore::close() {
  auto s = utils::Status::ok();
  for (uint32_t i = 0; i < kConcurLevel; ++i) {
    s = mSegments[i]->mBucketKVStore.close();
    if (!s.isOK()) {
      break;
    }
  }
  return s;
}

utils::Status ConcurInMemoryKVStore::lock(
    const std::set<KeyType> &targetKeys,
    bool isExclusive,
    WSLookupFunc wsLookup) {
  /// the order of segment index gurantee that we won't have deadlock
  uint32_t indexFlags[kConcurLevel] = {0};
  for (auto &key : targetKeys) {
    auto index = calculateSegmentIndex(key);
    assert(index < kConcurLevel);
    indexFlags[index] = 1;
  }
  for (uint32_t i = 0; i < kConcurLevel; ++i) {
    if (indexFlags[i] == 0) {
      continue;
    }
    SPDLOG_INFO("debug: trying to lock {}, exclusive {}", i, isExclusive);
    if (isExclusive) {
      mSegments[i]->mMutex.lock();
    } else {
      mSegments[i]->mMutex.lock_shared();
    }
  }
  return utils::Status::ok();
}

utils::Status ConcurInMemoryKVStore::unlock(
    const std::set<KeyType> &targetKeys,
    bool isExclusive,
    WSLookupFunc wsLookup) {
  /// the order of segment index gurantee that we won't have deadlock
  uint32_t indexFlags[kConcurLevel] = {0};
  for (auto &key : targetKeys) {
    auto index = calculateSegmentIndex(key);
    assert(index < kConcurLevel);
    indexFlags[index] = 1;
  }
  for (uint32_t i = 0; i < kConcurLevel; ++i) {
    if (indexFlags[i] == 0) {
      continue;
    }
    if (isExclusive) {
      mSegments[i]->mMutex.unlock();
    } else {
      mSegments[i]->mMutex.unlock_shared();
    }
  }
  return utils::Status::ok();
}

size_t ConcurInMemoryKVStore::calculateSegmentIndex(const KeyType &key) {
  /// SPDLOG_INFO("debug: calculate segment index, concur level: {}, mask: {}, shift: {}, hash: {}, segment: {}",
  ///  kConcurLevel, kSegmentMask, kSegmentShift, mHashFunc(key), (mHashFunc(key) >> kSegmentShift) & kSegmentMask);
  return (mHashFunc(key) >> kSegmentShift) & kSegmentMask;
}

utils::Status ConcurInMemoryKVStore::writeKV(
    const KeyType &key,
    const ValueType &value,
    const VersionType &version,
    WSLookupFunc wsLookup) {
  auto index = calculateSegmentIndex(key);
  assert(index < kConcurLevel);
  return mSegments[index]->mBucketKVStore.writeKV(key, value, version);
}

utils::Status ConcurInMemoryKVStore::writeTTLKV(const KeyType &key,
                                                const ValueType &value,
                                                const VersionType &version,
                                                const TTLType &ttl,
                                                const utils::TimeType &deadline, WSLookupFunc wsLookup) {
  auto index = calculateSegmentIndex(key);
  assert(index < kConcurLevel);
  return mSegments[index]->mBucketKVStore.writeTTLKV(key, value, version, ttl, deadline);
}

/// TODO: remove ttl kv in cache eviction when no get
utils::Status ConcurInMemoryKVStore::readKV(
    const KeyType &key,
    ValueType *outValue,
    TTLType *outTTL,
    VersionType *outVersion,
    WSLookupFunc wsLookup) {
  auto index = calculateSegmentIndex(key);
  assert(index < kConcurLevel);
  return mSegments[index]->mBucketKVStore.readKV(key, outValue, outTTL, outVersion);
}

utils::Status ConcurInMemoryKVStore::deleteKV(
    const KeyType &key,
    const VersionType &deleteRecordVersion,
    WSLookupFunc wsLookup) {
  auto index = calculateSegmentIndex(key);
  assert(index < kConcurLevel);
  return mSegments[index]->mBucketKVStore.deleteKV(key, deleteRecordVersion);
}

utils::Status ConcurInMemoryKVStore::readMeta(const KeyType &key, proto::Meta *meta, WSLookupFunc wsLookup) {
  auto index = calculateSegmentIndex(key);
  assert(index < kConcurLevel);
  return mSegments[index]->mBucketKVStore.readMeta(key, meta);
}

utils::Status ConcurInMemoryKVStore::commit(const MilestoneType &milestone, WSLookupFunc wsLookup) {
  /// not supported
  assert(0);
}

utils::Status ConcurInMemoryKVStore::loadMilestone(MilestoneType *milestone, WSLookupFunc wsLookup) {
  /// not supported
  assert(0);
}

void ConcurInMemoryKVStore::clear() {
  for (uint32_t i = 0; i < kConcurLevel; ++i) {
    mSegments[i]->mBucketKVStore.clear();
  }
}

utils::Status ConcurInMemoryKVStore::evictKV(
    const std::set<store::KeyType> &keys,
    store::VersionType guardVersion,
    WSLookupFunc wsLookup) {
  for (auto key : keys) {
    auto index = calculateSegmentIndex(key);
    assert(index < kConcurLevel);
    mSegments[index]->mBucketKVStore.evictKV({key}, guardVersion);
  }
  // trigger callback at here
  onEvictKeys(keys);
  return utils::Status::ok();
}

}  /// namespace goblin::kvengine::store


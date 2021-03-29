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

#ifndef SERVER_SRC_KV_ENGINE_STORE_CONCURINMEMORYKVSTORE_H_
#define SERVER_SRC_KV_ENGINE_STORE_CONCURINMEMORYKVSTORE_H_

#include <map>
#include <set>
#include <shared_mutex>

#include "InMemoryKVStore.h"
#include "ProxyKVStore.h"
#include "ReadOnlyKVStore.h"

namespace goblin::kvengine::store {

class ConcurInMemoryKVStore : public ProxyKVStore {
 public:
  explicit ConcurInMemoryKVStore(std::shared_ptr<ReadOnlyKVStore> delegateKVStore);
  virtual ~ConcurInMemoryKVStore() = default;

  utils::Status open() override;
  utils::Status close() override;

  utils::Status lock(const std::set<KeyType> &targetKeys,
                     bool isExclusive,
                     WSLookupFunc wsLookup = nullptr) override;
  utils::Status unlock(const std::set<KeyType> &targetKeys,
                       bool isExclusive,
                       WSLookupFunc wsLookup = nullptr) override;

  size_t calculateSegmentIndex(const KeyType &key);

  utils::Status writeKV(const KeyType &key,
                        const ValueType &value,
                        const VersionType &version,
                        WSLookupFunc wsLookup = nullptr) override;
  utils::Status writeTTLKV(const KeyType &key,
                           const ValueType &value,
                           const VersionType &version,
                           const TTLType &ttl,
                           const utils::TimeType &deadline,
                           WSLookupFunc wsLookup = nullptr) override;
  utils::Status readKV(const KeyType &key,
                       ValueType *outValue,
                       TTLType *outTTL,
                       VersionType *outVersion,
                       WSLookupFunc wsLookup = nullptr) override;
  utils::Status deleteKV(
      const KeyType &key,
      const VersionType &deleteRecordVersion,
      WSLookupFunc wsLookup = nullptr) override;
  utils::Status readMeta(const KeyType &key, proto::Meta *meta, WSLookupFunc wsLookup = nullptr) override;
  utils::Status commit(const MilestoneType &milestone, WSLookupFunc wsLookup = nullptr) override;
  utils::Status loadMilestone(MilestoneType *milestone, WSLookupFunc wsLookup = nullptr) override;
  utils::Status evictKV(
      const std::set<store::KeyType> &keys,
      store::VersionType guardVersion,
      WSLookupFunc wsLookup = nullptr) override;

  void clear() override;

  /**
 * the observer will be notified whenever a read/write op is performed
 */
  utils::Status registerObserver(
      const std::shared_ptr<KVObserver> &observer,
      WSLookupFunc wsLookup = nullptr) override {
    for (auto &segment : mSegments) segment->mBucketKVStore.registerObserver(observer);
    assert(KVStore::registerObserver(observer).isOK());
    return utils::Status::ok();
  }

 private:
  struct Segment {
    explicit Segment(std::shared_ptr<ReadOnlyKVStore> delegateKVStor) : mBucketKVStore(delegateKVStor) {}
    ~Segment() {}
    std::shared_mutex mMutex;
    InMemoryKVStore mBucketKVStore;
  };
  static constexpr uint32_t kConcurLevel = 16;
  static constexpr uint32_t kSegmentMask = kConcurLevel - 1;
  static constexpr uint32_t kSegmentShift = 64 - 4;

  std::unique_ptr<Segment> mSegments[kConcurLevel];
  std::hash<KeyType> mHashFunc;
};

}  /// namespace goblin::kvengine::store

#endif  // SERVER_SRC_KV_ENGINE_STORE_CONCURINMEMORYKVSTORE_H_


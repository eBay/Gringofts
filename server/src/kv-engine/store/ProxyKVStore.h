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

#ifndef SERVER_SRC_KV_ENGINE_STORE_PROXYKVSTORE_H_
#define SERVER_SRC_KV_ENGINE_STORE_PROXYKVSTORE_H_

#include <memory>

#include "KVStore.h"

namespace goblin::kvengine::store {

/// by default, this proxy store will invoke methods of delegateKVStore
/// but children classes can override to define its logic implementation
class ProxyKVStore : public KVStore {
 public:
  explicit ProxyKVStore(std::shared_ptr<KVStore> delegateKVStore) : mDelegateKVStore(delegateKVStore) {}

  virtual ~ProxyKVStore() = default;

  utils::Status open() override {
    return mDelegateKVStore->open();
  }
  utils::Status close() override {
    return mDelegateKVStore->close();
  }
  utils::Status writeKV(const KeyType &key,
      const ValueType &value,
      const VersionType &version,
      WSLookupFunc wsLookup = nullptr) override {
    return mDelegateKVStore->writeKV(key, value, version, wsLookup);
  }
  utils::Status writeTTLKV(const KeyType &key,
                                   const ValueType &value,
                                   const VersionType &version,
                                   const TTLType &ttl,
                                   const utils::TimeType &deadline,
                                   WSLookupFunc wsLookup = nullptr) override {
    return mDelegateKVStore->writeTTLKV(key, value, version, ttl, deadline, nullptr);
  }
  utils::Status readKV(const KeyType &key,
                               ValueType *outValue,
                               TTLType *outTTL,
                               VersionType *outVersion,
                               WSLookupFunc wsLookup = nullptr) override {
    return mDelegateKVStore->readKV(key, outValue, outTTL, outVersion, wsLookup);
  }
  utils::Status deleteKV(
      const KeyType &key,
      const VersionType &deleteRecordVersion,
      WSLookupFunc wsLookup = nullptr) override {
    return mDelegateKVStore->deleteKV(key, deleteRecordVersion, wsLookup);
  }
  utils::Status readMeta(const KeyType &key,
                                 proto::Meta *meta,
                                 WSLookupFunc wsLookup = nullptr) override {
    return mDelegateKVStore->readMeta(key, meta, wsLookup);
  }
  utils::Status commit(const MilestoneType &milestone, WSLookupFunc wsLookup = nullptr) override {
    return mDelegateKVStore->commit(milestone, wsLookup);
  }
  utils::Status loadMilestone(MilestoneType *milestone, WSLookupFunc wsLookup = nullptr) override {
    return mDelegateKVStore->loadMilestone(milestone, wsLookup);
  }

  void clear() override {
    return mDelegateKVStore->clear();
  }

  std::shared_ptr<SnapshottedKVStore> takeSnapshot() override {
    return mDelegateKVStore->takeSnapshot();
  }

 protected:
  std::shared_ptr<KVStore> mDelegateKVStore;
};

}  /// namespace goblin::kvengine::store

#endif  // SERVER_SRC_KV_ENGINE_STORE_PROXYKVSTORE_H_


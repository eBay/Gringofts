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

#ifndef SERVER_SRC_KV_ENGINE_STORE_READONLYKVSTORE_H_
#define SERVER_SRC_KV_ENGINE_STORE_READONLYKVSTORE_H_

#include <map>

#include "ProxyKVStore.h"

namespace goblin::kvengine::store {

class ReadOnlyKVStore: public ProxyKVStore {
 public:
  explicit ReadOnlyKVStore(std::shared_ptr<KVStore> delegateKVStore): ProxyKVStore(delegateKVStore) {}
  virtual ~ReadOnlyKVStore() = default;

  utils::Status writeKV(const KeyType &key,
                        const ValueType &value,
                        const VersionType &version,
                        WSLookupFunc wsLookup = nullptr) override {
    /// not supported
    assert(0);
  }
  utils::Status writeTTLKV(const KeyType &key,
                           const ValueType &value,
                           const VersionType &version,
                           const TTLType &ttl,
                           const utils::TimeType& deadline,
                           WSLookupFunc wsLookup = nullptr) override {
    /// not supported
    assert(0);
  }
  utils::Status deleteKV(
      const KeyType &key,
      const VersionType &deleteRecordVersion,
      WSLookupFunc wsLookup = nullptr) override {
    /// not supported
    assert(0);
  }

  utils::Status commit(const MilestoneType &milestone, WSLookupFunc wsLookup = nullptr) override {
    /// not supported
    assert(0);
  }

  utils::Status loadMilestone(MilestoneType *milestone, WSLookupFunc wsLookup = nullptr) override {
    /// not supported
    assert(0);
  }

  void clear() override {
    /// not supported
    assert(0);
  }
};

}  /// namespace goblin::kvengine::store

#endif  // SERVER_SRC_KV_ENGINE_STORE_READONLYKVSTORE_H_


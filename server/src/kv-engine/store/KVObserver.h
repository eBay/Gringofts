/************************************************************************
Copyright 2020-2021 eBay Inc.
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

#ifndef SERVER_SRC_KV_ENGINE_STORE_KVOBSERVER_H_
#define SERVER_SRC_KV_ENGINE_STORE_KVOBSERVER_H_

#include "../types.h"

namespace goblin::kvengine::store {

class KVObserver {
 public:
  KVObserver() = default;
  virtual ~KVObserver() = default;
  virtual void onReadValue(const KeyType &key) = 0;
  virtual void onDeleteKey(const KeyType &key, const VersionType &version, const VersionType &deleteRecordVersion) = 0;
  virtual void onEvictKeys(const std::set<store::KeyType> &keys) = 0;
  virtual void onWriteValue(const KeyType &key, const ValueType &value, const VersionType &version) = 0;
};

}  /// namespace goblin::kvengine::store

#endif  // SERVER_SRC_KV_ENGINE_STORE_KVOBSERVER_H_


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


#include "WSConcurInMemoryKVStore.h"

#include <spdlog/spdlog.h>

namespace goblin::kvengine::store {

WSConcurInMemoryKVStore::WSConcurInMemoryKVStore(
    std::shared_ptr<ReadOnlyKVStore> delegateKVStore,
    const std::vector<WSName> &initWSNames,
    WSLookupFunc defaultWSLookupFunc) :
  ProxyKVStore(delegateKVStore),
  mDefaultWSLookupFunc(defaultWSLookupFunc) {
  for (auto name : initWSNames) {
    assert(mWS2Wrapper.find(name) == mWS2Wrapper.end());
    mWS2Wrapper[name].mKVStore = std::make_unique<ConcurInMemoryKVStore>(delegateKVStore);
  }
}

utils::Status WSConcurInMemoryKVStore::open() {
  auto s = utils::Status::ok();
  for (auto &[wsName, wrapper] : mWS2Wrapper) {
    s = wrapper.mKVStore->open();
    if (!s.isOK()) {
      break;
    }
  }
  return s;
}

utils::Status WSConcurInMemoryKVStore::close() {
  auto s = utils::Status::ok();
  for (auto &[wsName, wrapper] : mWS2Wrapper) {
    s = wrapper.mKVStore->close();
    if (!s.isOK()) {
      break;
    }
  }
  return s;
}

utils::Status WSConcurInMemoryKVStore::lock(
    const std::set<KeyType> &targetKeys,
    bool isExclusive,
    WSLookupFunc wsLookup) {
  std::map<WSName, std::set<KeyType>> ws2Keys;
  for (auto &key : targetKeys) {
    auto name = mDefaultWSLookupFunc(key);
    auto it = ws2Keys.find(name);
    if (it != ws2Keys.end()) {
      it->second.insert(key);
    } else {
      ws2Keys[name] = { key };
    }
  }
  for (auto &[name, keys] : ws2Keys) {
    assert(mWS2Wrapper.find(name) != mWS2Wrapper.end());
    mWS2Wrapper[name].mKVStore->lock(keys, isExclusive, wsLookup);
  }
  return utils::Status::ok();
}

utils::Status WSConcurInMemoryKVStore::lockWS(const WSName &targetWS, bool isExclusive) {
  assert(mWS2Wrapper.find(targetWS) != mWS2Wrapper.end());
  if (isExclusive) {
    mWS2Wrapper[targetWS].mMutex.lock();
  } else {
    mWS2Wrapper[targetWS].mMutex.lock_shared();
  }
  return utils::Status::ok();
}

utils::Status WSConcurInMemoryKVStore::unlock(
    const std::set<KeyType> &targetKeys,
    bool isExclusive, WSLookupFunc wsLookup) {
  std::map<WSName, std::set<KeyType>> ws2Keys;
  for (auto &key : targetKeys) {
    auto name = mDefaultWSLookupFunc(key);
    auto it = ws2Keys.find(name);
    if (it != ws2Keys.end()) {
      it->second.insert(key);
    } else {
      ws2Keys[name] = { key };
    }
  }
  for (auto &[name, keys] : ws2Keys) {
    assert(mWS2Wrapper.find(name) != mWS2Wrapper.end());
    mWS2Wrapper[name].mKVStore->unlock(keys, isExclusive, wsLookup);
  }
  return utils::Status::ok();
}

utils::Status WSConcurInMemoryKVStore::unlockWS(const WSName &targetWS, bool isExclusive) {
  assert(mWS2Wrapper.find(targetWS) != mWS2Wrapper.end());
  if (isExclusive) {
    mWS2Wrapper[targetWS].mMutex.unlock();
  } else {
    mWS2Wrapper[targetWS].mMutex.unlock_shared();
  }
  return utils::Status::ok();
}

utils::Status WSConcurInMemoryKVStore::writeKV(
    const KeyType &key,
    const ValueType &value,
    const VersionType &version, WSLookupFunc wsLookup) {
  auto name = mDefaultWSLookupFunc(key);
  assert(mWS2Wrapper.find(name) != mWS2Wrapper.end());
  return mWS2Wrapper[name].mKVStore->writeKV(key, value, version);
}

utils::Status WSConcurInMemoryKVStore::writeTTLKV(const KeyType &key,
                                                const ValueType &value,
                                                const VersionType &version,
                                                const TTLType &ttl,
                                                const utils::TimeType &deadline, WSLookupFunc wsLookup) {
  auto name = mDefaultWSLookupFunc(key);
  assert(mWS2Wrapper.find(name) != mWS2Wrapper.end());
  return mWS2Wrapper[name].mKVStore->writeTTLKV(key, value, version, ttl, deadline);
}

/// TODO: remove ttl kv in cache eviction when no get
utils::Status WSConcurInMemoryKVStore::readKV(
    const KeyType &key,
    ValueType *outValue,
    TTLType *outTTL,
    VersionType *outVersion,
    WSLookupFunc wsLookup) {
  auto name = mDefaultWSLookupFunc(key);
  assert(mWS2Wrapper.find(name) != mWS2Wrapper.end());
  return mWS2Wrapper[name].mKVStore->readKV(key, outValue, outTTL, outVersion);
}

utils::Status WSConcurInMemoryKVStore::deleteKV(
    const KeyType &key,
    const VersionType &deleteRecordVersion,
    WSLookupFunc wsLookup) {
  auto name = mDefaultWSLookupFunc(key);
  assert(mWS2Wrapper.find(name) != mWS2Wrapper.end());
  return mWS2Wrapper[name].mKVStore->deleteKV(key, deleteRecordVersion);
}

utils::Status WSConcurInMemoryKVStore::readMeta(const KeyType &key, proto::Meta *meta, WSLookupFunc wsLookup) {
  auto name = mDefaultWSLookupFunc(key);
  assert(mWS2Wrapper.find(name) != mWS2Wrapper.end());
  return mWS2Wrapper[name].mKVStore->readMeta(key, meta);
}

utils::Status WSConcurInMemoryKVStore::commit(const MilestoneType &milestone, WSLookupFunc wsLookup) {
  /// not supported
  assert(0);
}

utils::Status WSConcurInMemoryKVStore::loadMilestone(MilestoneType *milestone, WSLookupFunc wsLookup) {
  /// not supported
  assert(0);
}

void WSConcurInMemoryKVStore::clear() {
  for (auto &[wsName, wrapper] : mWS2Wrapper) {
    wrapper.mKVStore->clear();
  }
}

utils::Status WSConcurInMemoryKVStore::evictKV(
    const std::set<store::KeyType> &keys,
    store::VersionType guardVersion,
    WSLookupFunc wsLookup) {
  for (auto key : keys) {
    auto name = mDefaultWSLookupFunc(key);
    assert(mWS2Wrapper.find(name) != mWS2Wrapper.end());
    mWS2Wrapper[name].mKVStore->evictKV({key}, guardVersion);
  }
  // trigger callback at here
  onEvictKeys(keys);
  return utils::Status::ok();
}

}  /// namespace goblin::kvengine::store


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


#include "Event.h"

#include <rocksdb/db.h>
#include <spdlog/spdlog.h>

#include "../store/VersionStore.h"

namespace goblin::kvengine::model {

using utils::Status;

Status Event::toBundle(const EventList &events, goblin::proto::Bundle* outBundle) {
  for (auto &e : events) {
    auto newProtoEvent = outBundle->add_events();
    e->toProtoEvent(newProtoEvent);
  }
  return Status::ok();
}

Status Event::fromBundle(const proto::Bundle &bundle, EventList *outEvents) {
  for (auto &e : bundle.events()) {
    if (e.has_writeevent()) {
      outEvents->emplace_back(std::make_unique<WriteEvent>(e.writeevent()));
    } else if (e.has_deleteevent()) {
      outEvents->emplace_back(std::make_unique<DeleteEvent>(e.deleteevent()));
    } else {
      assert(0);
    }
  }
  return Status::ok();
}

Status Event::getVersionFromBundle(const proto::Bundle &bundle, store::VersionType *version) {
  assert(bundle.events().size() > 0);
  auto e = bundle.events().Get(0);
  if (e.has_writeevent()) {
    *version = e.writeevent().version();
  } else if (e.has_deleteevent()) {
    *version = e.deleteevent().version();
  } else {
    assert(0);
  }
  return Status::ok();
}

Status WriteEvent::apply(store::KVStore &kvStore) {
  utils::Status s = utils::Status::ok();
  if (this->mEnableTTL) {
    s = kvStore.writeTTLKV(mKey, mValue, mVersion, mTTL, mDeadline);
    /// SPDLOG_INFO("debug: write ttl key {},
    /// value {}, version {}, ttl {}, deadline {}", mKey, mValue, mVersion, mTTL, mDeadline);
  } else {
    s = kvStore.writeKV(mKey, mValue, mVersion);
    /// SPDLOG_INFO("debug: write key {}, value {}, version {}", mKey, mValue, mVersion);
  }
  return s;
}

Status WriteEvent::toProtoEvent(goblin::proto::Event *outEvent) {
  auto writeEvent = outEvent->mutable_writeevent();
  writeEvent->set_key(mKey);
  writeEvent->set_value(mValue);
  writeEvent->set_version(mVersion);
  writeEvent->set_enablettl(mEnableTTL);
  writeEvent->set_ttl(mTTL);
  writeEvent->set_deadline(mDeadline);
  return Status::ok();
}

void WriteEvent::assignVersion(const store::VersionType &version) {
  assert(mVersion == store::VersionStore::kInvalidVersion);
  mVersion = version;
}

const store::VersionType& WriteEvent::getAllocatedVersion() const {
  return mVersion;
}

const store::KeyType& WriteEvent::getKey() const {
  return mKey;
}

Status ReadEvent::apply(store::KVStore &kvStore) {
  /// for read, do nothing
  return utils::Status::ok();
}

Status ReadEvent::toProtoEvent(goblin::proto::Event *outEvent) {
  /// not supported
  assert(0);
}

const store::KeyType& ReadEvent::getKey() const {
  return mKey;
}

const store::ValueType& ReadEvent::getValue() const {
  return mValue;
}

const store::VersionType& ReadEvent::getValueVersion() const {
  return mVersion;
}

bool ReadEvent::isNotFound() const {
  return mIsNotFound;
}

Status DeleteEvent::apply(store::KVStore &kvStore) {
  kvStore.deleteKV(mKey, mVersion);
  /// SPDLOG_INFO("debug: delete key {}", mKey);
  return Status::ok();
}

Status DeleteEvent::toProtoEvent(goblin::proto::Event *outEvent) {
  auto deleteEvent = outEvent->mutable_deleteevent();
  deleteEvent->set_key(mKey);
  deleteEvent->set_deletedversion(mDeletedVersion);
  deleteEvent->set_version(mVersion);
  return Status::ok();
}

void DeleteEvent::assignVersion(const store::VersionType &version) {
  assert(mVersion == store::VersionStore::kInvalidVersion);
  mVersion = version;
}
const store::VersionType& DeleteEvent::getAllocatedVersion() const {
  return mVersion;
}

const store::ValueType& DeleteEvent::getDeletedValue() const {
  return mDeletedValue;
}

const store::VersionType& DeleteEvent::getDeletedVersion() const {
  return mDeletedVersion;
}

}  /// namespace goblin::kvengine::model


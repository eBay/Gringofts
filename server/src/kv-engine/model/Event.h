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

#ifndef SERVER_SRC_KV_ENGINE_MODEL_EVENT_H_
#define SERVER_SRC_KV_ENGINE_MODEL_EVENT_H_

#include "../../../protocols/generated/model.pb.h"
#include "../store/KVStore.h"
#include "../store/VersionStore.h"

namespace goblin::kvengine::model {

enum class EventType {
  READ = 0,
  WRITE = 1,
  DELETE = 2,
  APPEND = 3,
  /// for shard migration
  IMPORT = 4,
};

class Event;

using EventList = std::vector<std::shared_ptr<model::Event>>;

class Event {
 public:
  explicit Event(EventType type): mType(type) {}
  virtual ~Event() = default;

  EventType getType() { return mType; }

  virtual utils::Status apply(store::KVStore &kvStore) = 0;  // NOLINT(runtime/references)
  virtual void assignVersion(const store::VersionType &version) {
     /// ignore by default
  }

  /// from a model event to a proto event
  virtual utils::Status toProtoEvent(proto::Event *outEvent) = 0;
  /// from model events to a proto bundle
  static utils::Status toBundle(const EventList &events, proto::Bundle* outBundle);
  /// from a proto bundle to model events
  static utils::Status fromBundle(const proto::Bundle &bundle, EventList *outEvents);
  static utils::Status getVersionFromBundle(const proto::Bundle &bundle, store::VersionType *version);

 private:
  EventType mType;
};

class WriteEvent: public Event {
 public:
  WriteEvent(
      const store::KeyType &key,
      const store::ValueType &value,
      bool enableTTL = false,
      const store::TTLType &ttl = store::INFINITE_TTL,
      const utils::TimeType& deadline = store::NEVER_EXPIRE):
     Event(EventType::WRITE),
     mKey(key),
     mValue(value),
     mEnableTTL(enableTTL),
     mTTL(ttl),
     mDeadline(deadline) {}

  explicit WriteEvent(const proto::WriteEvent &event):
     Event(EventType::WRITE),
     mKey(event.key()),
     mValue(event.value()),
     mVersion(event.version()),
     mEnableTTL(event.enablettl()),
     mTTL(event.ttl()),
     mDeadline(event.deadline()) {}

  void assignVersion(const store::VersionType &version) override;
  const store::VersionType& getAllocatedVersion() const;
  const store::KeyType& getKey() const;

  utils::Status apply(store::KVStore &kvStore) override;

  utils::Status toProtoEvent(proto::Event *outEvent) override;

 private:
  store::KeyType mKey;
  store::ValueType mValue;
  store::VersionType mVersion = store::VersionStore::kInvalidVersion;
  bool mEnableTTL;
  store::TTLType mTTL;
  utils::TimeType mDeadline;
};

class ReadEvent: public Event {
 public:
  ReadEvent(
      const store::KeyType &key,
      const store::ValueType &value,
      const store::TTLType &ttl,
      const store::VersionType &version,
      bool isNotFound):
     Event(EventType::READ),
     mKey(key),
     mValue(value),
     mTTL(ttl),
     mVersion(version),
     mIsNotFound(isNotFound) {}

  const store::KeyType& getKey() const;
  const store::ValueType& getValue() const;
  const store::TTLType& getTTL() const;
  const store::VersionType& getValueVersion() const;
  bool isNotFound() const;

  utils::Status apply(store::KVStore &kvStore) override;  // NOLINT(runtime/references)

  utils::Status toProtoEvent(proto::Event *outEvent) override;

 private:
  store::KeyType mKey;
  store::ValueType mValue;
  store::TTLType mTTL;
  /// this is the read version
  store::VersionType mVersion;
  bool mIsNotFound = false;
};

class DeleteEvent: public Event {
 public:
  DeleteEvent(
       const store::KeyType &key,
       const store::ValueType &deletedValue,
       const store::VersionType &deletedVersion):
     Event(EventType::DELETE),
     mKey(key),
     mDeletedValue(deletedValue),
     mDeletedVersion(deletedVersion) {}

  explicit DeleteEvent(const proto::DeleteEvent &event):
     Event(EventType::DELETE),
     mKey(event.key()),
     mDeletedValue(event.deletedvalue()),
     mDeletedVersion(event.deletedversion()),
     mVersion(event.version()) {}

  void assignVersion(const store::VersionType &version) override;
  const store::VersionType& getAllocatedVersion() const;
  const store::ValueType& getDeletedValue() const;
  const store::VersionType& getDeletedVersion() const;

  utils::Status apply(store::KVStore &kvStore) override;  // NOLINT(runtime/references)

  utils::Status toProtoEvent(proto::Event *outEvent) override;

 private:
  store::KeyType mKey;
  /// the value that is deleted, if users don't need it, this could be empty
  store::ValueType mDeletedValue;
  /// the version that is deleted
  store::VersionType mDeletedVersion;
  /// the version that belongs to this delete log entry
  store::VersionType mVersion = store::VersionStore::kInvalidVersion;
};

class ImportEvent : public Event {
 public:
  ImportEvent(
      const store::VersionType &importedVersion,
      const store::KeyType &key,
      const store::ValueType &value,
      bool enableTTL = false,
      const store::TTLType &ttl = store::INFINITE_TTL,
      const utils::TimeType& deadline = store::NEVER_EXPIRE):
     Event(EventType::IMPORT),
     mWriteEvent(key, value, enableTTL, ttl, deadline),
     mImportedVersion(importedVersion) {}

  explicit ImportEvent(const proto::WriteEvent &event):
     Event(EventType::IMPORT),
     mWriteEvent(event) {}

  void assignVersion(const store::VersionType &version) override {
     mWriteEvent.assignVersion(version);
  }
  const store::VersionType& getAllocatedVersion() const {
     return mWriteEvent.getAllocatedVersion();
  }
  const store::KeyType& getKey() const {
     return mWriteEvent.getKey();
  }

  utils::Status apply(store::KVStore &kvStore) override {  // NOLINT(runtime/references)
     return mWriteEvent.apply(kvStore);
  }

  utils::Status toProtoEvent(proto::Event *outEvent) override {
     return mWriteEvent.toProtoEvent(outEvent);
  }

  store::VersionType getImportedVersion() const {
     return mImportedVersion;
  }

 private:
  WriteEvent mWriteEvent;
  store::VersionType mImportedVersion = store::VersionStore::kInvalidVersion;
};

}  /// namespace goblin::kvengine::model

#endif  // SERVER_SRC_KV_ENGINE_MODEL_EVENT_H_


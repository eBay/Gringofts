/************************************************************************
Copyright 2019-2020 eBay Inc.
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

#ifndef SRC_INFRA_ES_EVENT_H_
#define SRC_INFRA_ES_EVENT_H_

#include "../Decodable.h"
#include "../Encodable.h"
#include "../common_types.h"
#include "../util/TimeUtil.h"
#include "EventMetaData.h"

namespace gringofts {

/**
 * A tag interface representing any immutable events as the result of
 * the execution of #gringofts::Command
 * (See #gringofts::ProcessCommandStateMachine.processCommand).
 *
 * Event name must be in past tense, e.g., AccountTransferredEvent.
 *
 * Event is something happened in the past, so it must be immutable,
 * i.e., once constructed, it cannot be modified.
 *
 * Only when an event is persisted in #gringofts::CommandEventStore it becomes a fact,
 * which will be applied in #gringofts::StateMachine, whose internal state
 * will then get updated accordingly.
 */
class Event : public Encodable, Decodable {
 public:
  Event(Type type, TimestampInNanos createdTimeInNanos) {
    // mId and mCommandId are set when this event is being persisted
    mMetaData.setType(type);
    mMetaData.setCreatedTimeInNanos(createdTimeInNanos);
  }
  virtual ~Event() = default;

  // setters
  void setId(Id id) { mMetaData.setId(id); }
  void setCommandId(Id commandId) { mMetaData.setCommandId(commandId); }
  void setCreatorId(Id creatorId) { mMetaData.setCreatorId(creatorId); }
  void setGroupId(Id groupId) { mMetaData.setGroupId(groupId); }
  void setGroupVersion(uint64_t groupVersion) { mMetaData.setGroupVersion(groupVersion); }
  void setTrackingContext(std::string trackingContext) { mMetaData.setTrackingContext(trackingContext); }

  // type and createdTimeInNanos has been initialized in the constructor, ignore here
  void setPartialMetaData(const EventMetaData &metaData) {
    setId(metaData.getId());
    setCommandId(metaData.getCommandId());
    setCreatorId(metaData.getCreatorId());
    setGroupId(metaData.getGroupId());
    setGroupVersion(metaData.getGroupVersion());
    setTrackingContext(metaData.getTrackingContext());
  }

  // getters
  Id getId() const { return mMetaData.getId(); }
  Type getType() const { return mMetaData.getType(); }
  Id getCommandId() const { return mMetaData.getCommandId(); }
  TimestampInNanos getCreatedTimeInNanos() const { return mMetaData.getCreatedTimeInNanos(); }
  Id getCreatorId() const { return mMetaData.getCreatorId(); }
  Id getGroupId() const { return mMetaData.getGroupId(); }
  uint64_t getGroupVersion() const { return mMetaData.getGroupVersion(); }
  std::string getTrackingContext() const { return mMetaData.getTrackingContext(); }
  const EventMetaData &getMetaData() const { return mMetaData; }

  /**
   * Compare equality
   */
  virtual bool equals(const Event &another) const {
    return encodeToString() == another.encodeToString();
  }

 protected:
  /**
   * Holds all the meta data-related info. MetaData can be changed after event is created.
   */
  EventMetaData mMetaData;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_EVENT_H_

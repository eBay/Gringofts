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

#ifndef SRC_INFRA_ES_EVENTMETADATA_H_
#define SRC_INFRA_ES_EVENTMETADATA_H_

#include "../common_types.h"
#include "../util/TimeUtil.h"
#include "MetaData.h"

namespace gringofts {

/**
 * Hold all the metadata of a #gringofts::Event instance.
 * Fields in the metadata can be set after the event is created,
 * e.g., Id is set when the event instance is being persisted.
 */
class EventMetaData : public MetaData {
 public:
  EventMetaData() = default;

  explicit EventMetaData(const es::EventEntry &eventEntry) : MetaData(eventEntry),
                                                             mCommandId(eventEntry.commandid()) {}

  ~EventMetaData() = default;

  // getters
  Id getCommandId() const {
    return mCommandId;
  }

  // setters
  void setCommandId(Id commandId) {
    mCommandId = commandId;
  }

  void populateEventEntry(es::EventEntry *eventEntry) const {
    eventEntry->set_type(getType());
    eventEntry->set_id(getId());
    eventEntry->set_commandid(getCommandId());
    eventEntry->set_createdtimeinnanos(getCreatedTimeInNanos());
    eventEntry->set_creatorid(getCreatorId());
    eventEntry->set_groupid(getGroupId());
    eventEntry->set_groupversion(getGroupVersion());
    eventEntry->set_trackingcontext(getTrackingContext());
  }

 private:
  /**
   * Used to refer to the instance of #gringofts::Command after the processing of which generated this event
   */
  Id mCommandId;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_EVENTMETADATA_H_

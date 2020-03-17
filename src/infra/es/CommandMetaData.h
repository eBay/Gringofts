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

#ifndef SRC_INFRA_ES_COMMANDMETADATA_H_
#define SRC_INFRA_ES_COMMANDMETADATA_H_

#include "../common_types.h"
#include "../grpc/RequestHandle.h"
#include "../util/TimeUtil.h"
#include "MetaData.h"

namespace gringofts {

/**
 * Hold all the metadata of a #gringofts::Command instance.
 * Fields in the metadata can be set after the command is created,
 * e.g., Id is set when the command instance is being persisted.
 */
class CommandMetaData : public MetaData {
 public:
  CommandMetaData() = default;
  explicit CommandMetaData(const es::CommandEntry &commandEntry) : MetaData(commandEntry) {}
  ~CommandMetaData() = default;

  // getters
  RequestHandle *getRequestHandle() const {
    return mRequestHandle;
  }

  // setters
  void setRequestHandle(RequestHandle *requestHandle) {
    CommandMetaData::mRequestHandle = requestHandle;
  }

  void populateCommandEntry(es::CommandEntry *const commandEntry) const {
    commandEntry->set_type(getType());
    commandEntry->set_id(getId());
    commandEntry->set_createdtimeinnanos(getCreatedTimeInNanos());
    commandEntry->set_creatorid(getCreatorId());
    commandEntry->set_groupid(getGroupId());
    commandEntry->set_groupversion(getGroupVersion());
    commandEntry->set_trackingcontext(getTrackingContext());
  }

 private:
  /**
   * To uniquely identify the corresponding request
   */
  RequestHandle *mRequestHandle = nullptr;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_COMMANDMETADATA_H_

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

#ifndef SRC_INFRA_ES_METADATA_H_
#define SRC_INFRA_ES_METADATA_H_

#include "../common_types.h"
#include "../util/TimeUtil.h"
#include "store/generated/store.pb.h"

namespace gringofts {

class MetaData {
 public:
  MetaData() = default;
  explicit MetaData(const es::EventEntry &eventEntry) : mId(eventEntry.id()),
                                                        mType(eventEntry.type()),
                                                        mCreatedTimeInNanos(eventEntry.createdtimeinnanos()),
                                                        mCreatorId(eventEntry.creatorid()),
                                                        mGroupId(eventEntry.groupid()),
                                                        mGroupVersion(eventEntry.groupversion()),
                                                        mTrackingContext(eventEntry.trackingcontext()) {}

  explicit MetaData(const es::CommandEntry &commandEntry) : mId(commandEntry.id()),
                                                            mType(commandEntry.type()),
                                                            mCreatedTimeInNanos(
                                                                commandEntry.createdtimeinnanos()),
                                                            mCreatorId(commandEntry.creatorid()),
                                                            mGroupId(commandEntry.groupid()),
                                                            mGroupVersion(commandEntry.groupversion()),
                                                            mTrackingContext(commandEntry.trackingcontext()) {}
  virtual ~MetaData() = default;

  // getters
  const Id &getId() const {
    return mId;
  }

  const Type &getType() const {
    return mType;
  }

  const TimestampInNanos &getCreatedTimeInNanos() const {
    return mCreatedTimeInNanos;
  }

  const TimestampInNanos &getProcessTimeInNanos() const {
    return mProcessTimeInNanos;
  }

  const TimestampInNanos &getLeaderReadyTimeInNanos() const {
    return mLeaderReadyTimeInNanos;
  }

  const TimestampInNanos &getFinishTimeInNanos() const {
    return mFinishTimeInNanos;
  }

  const Id &getCreatorId() const {
    return mCreatorId;
  }

  const Id &getGroupId() const {
    return mGroupId;
  }

  const uint64_t &getGroupVersion() const {
    return mGroupVersion;
  }

  const std::string &getTrackingContext() const {
    return mTrackingContext;
  }

  // setters
  void setId(Id id) {
    mId = id;
  }

  void setType(Type type) {
    mType = type;
  }

  void setCreatedTimeInNanos(TimestampInNanos createdTimeInNanos) {
    mCreatedTimeInNanos = createdTimeInNanos;
  }

  void setProcessTimeInNanos(TimestampInNanos processTimeInNanos) {
    mProcessTimeInNanos = processTimeInNanos;
  }

  void setLeaderReadyTimeInNanos(TimestampInNanos leaderReadyTimeInNanos) {
    mLeaderReadyTimeInNanos = leaderReadyTimeInNanos;
  }

  void setFinishTimeInNanos(TimestampInNanos finishTimeInNanos) {
    mFinishTimeInNanos = finishTimeInNanos;
  }

  void setCreatorId(Id creatorId) {
    mCreatorId = creatorId;
  }

  void setGroupId(Id groupId) {
    mGroupId = groupId;
  }

  void setGroupVersion(uint64_t groupVersion) {
    mGroupVersion = groupVersion;
  }

  void setTrackingContext(std::string trackingContext) {
    mTrackingContext = trackingContext;
  }

 protected:
  /**
   * Id as a unique reference to the specific instance.
   *
   * For #gringofts::Command, the id can be used by events to locate related command. A typical use scenario is crash/recover, where after
   * replaying all the events, the app will get the command id in the last applied event and get the command after it.
   *
   * For #gringofts::Event, the id can also be sent to the upstream which can later use it to query related events
   */
  Id mId;
  /**
   * Type is mainly used in transforming information stored in medias such as file into in-memory Command instance.
   *
   * Each subclass must have a unique type name.
   */
  Type mType;
  /**
   * Capture the time in nanos as of the instance has been created
   */
  TimestampInNanos mCreatedTimeInNanos = 0;
  TimestampInNanos mProcessTimeInNanos = 0;
  TimestampInNanos mLeaderReadyTimeInNanos = 0;
  TimestampInNanos mFinishTimeInNanos = 0;
  /**
   * Identify the creator that generates this instance. It can be interpreted as data source by downstream
   */
  Id mCreatorId;
  /**
   * Identify the partition (i.e., group) that the creator belongs to
   */
  Id mGroupId;
  /**
   * Each creator can belong to a different partition under different version
   */
  uint64_t mGroupVersion;
  /**
   * Json string which record the context info for tracking purpose.
   *
   * It should be persisted for raft-based store in order to enable tracking in staging/production environment.
   * While for SQLite-based store it will not be persisted since SQLite-based store is mainly used for local test which tracking feature is not needed.
   */
  std::string mTrackingContext;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_METADATA_H_

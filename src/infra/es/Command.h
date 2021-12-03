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

#ifndef SRC_INFRA_ES_COMMAND_H_
#define SRC_INFRA_ES_COMMAND_H_

#include "../Decodable.h"
#include "../Encodable.h"
#include "../grpc/RequestHandle.h"
#include "../util/MetricReporter.h"
#include "../util/TimeUtil.h"
#include "CommandMetaData.h"

namespace gringofts {

/**
 * A tag interface to represents an action which once processed can generate
 * 0, 1 or multiple events (see #gringofts::Event).
 */
class Command : public Encodable, Decodable {
 public:
  /**
   * @param type if type < 100 is normal command, it type >=100 , it is ctrl command
   * @param createdTimeInNanos
   */
  Command(Type type, TimestampInNanos createdTimeInNanos) {
    // mMetaData.id will be persisted when this instance is being persisted
    mMetaData.setType(type);
    mMetaData.setCreatedTimeInNanos(createdTimeInNanos);
  }

  virtual ~Command() = default;

  // setters
  void setId(Id id) { mMetaData.setId(id); }

  void setProcessTimeInNanos(TimestampInNanos time) { return mMetaData.setProcessTimeInNanos(time); }
  void setLeaderReadyTimeInNanos(TimestampInNanos time) { return mMetaData.setLeaderReadyTimeInNanos(time); }
  void setFinishTimeInNanos(TimestampInNanos time) { return mMetaData.setFinishTimeInNanos(time); }

  void setCreatorId(Id creatorId) { mMetaData.setCreatorId(creatorId); }
  void setGroupId(Id groupId) { mMetaData.setGroupId(groupId); }
  void setGroupVersion(uint64_t groupVersion) { mMetaData.setGroupVersion(groupVersion); }
  void setTrackingContext(std::string trackingContext) { mMetaData.setTrackingContext(trackingContext); }
  void setRequestHandle(RequestHandle *requestHandle) {
    mMetaData.setRequestHandle(requestHandle);
  }
  virtual void reportMetrics() const {
    /// Created->Process->LeaderReady(optional)->Finish
    gringofts::MetricReporter::reportLatency("queue_latency_in_ms",
        mMetaData.getCreatedTimeInNanos(), mMetaData.getProcessTimeInNanos(), false, true, true);
    gringofts::MetricReporter::reportLatency("becomeLeader_latency_in_ms",
        mMetaData.getProcessTimeInNanos(), mMetaData.getLeaderReadyTimeInNanos(), false, true, true);
    if (mMetaData.getLeaderReadyTimeInNanos() == 0) {
      /// no leader election
      gringofts::MetricReporter::reportLatency("process_latency_in_ms",
          mMetaData.getProcessTimeInNanos(), mMetaData.getFinishTimeInNanos(), false, true, true);
    } else {
      /// has leader election
      gringofts::MetricReporter::reportLatency("process_latency_in_ms",
          mMetaData.getLeaderReadyTimeInNanos(), mMetaData.getFinishTimeInNanos(), false, true, true);
    }
  }

  // type, createdTimeInNanos and requestHandle have been initialized in the constructor, ignore here
  void setPartialMetaData(const CommandMetaData &metaData) {
    setId(metaData.getId());
    setCreatorId(metaData.getCreatorId());
    setGroupId(metaData.getGroupId());
    setGroupVersion(metaData.getGroupVersion());
    setTrackingContext(metaData.getTrackingContext());
  }

  // getters
  Id getId() const { return mMetaData.getId(); }
  Type getType() const { return mMetaData.getType(); }
  TimestampInNanos getCreatedTimeInNanos() const { return mMetaData.getCreatedTimeInNanos(); }
  TimestampInNanos getProcessTimeInNanos() const { return mMetaData.getProcessTimeInNanos(); }
  TimestampInNanos getLeaderReadyTimeInNanos() const { return mMetaData.getLeaderReadyTimeInNanos(); }
  TimestampInNanos getFinishTimeInNanos() const { return mMetaData.getFinishTimeInNanos(); }
  Id getCreatorId() const { return mMetaData.getCreatorId(); }
  Id getGroupId() const { return mMetaData.getGroupId(); }
  uint64_t getGroupVersion() const { return mMetaData.getGroupVersion(); }
  std::string getTrackingContext() const { return mMetaData.getTrackingContext(); }
  RequestHandle *getRequestHandle() const { return mMetaData.getRequestHandle(); }
  const CommandMetaData &getMetaData() const { return mMetaData; }

  static constexpr char kVerifiedSuccess[] = "Success";
  /**
   * Check whether the command is valid.
   * @return "Success" for valid or the error string.
   */
  virtual std::string verifyCommand() const {
    return kVerifiedSuccess;
  }

  /**
   * Get invoked when command has been successfully persisted to #gringofts::CommandEventStore.
   * @param message the optional message that will be returned. if nullptr, a default message will be returned.
   */
  virtual void onPersisted(const std::string &message = "Success") = 0;
  /**
   * Get invoked when command failed to be persisted to #gringofts::CommandEventStore.
   * @param errorMessage the error message explaining why the persist failed
   * @param reserved extra message. It's leaderHint if the persistence is backed by Raft.
   */
  virtual void onPersistFailed(uint32_t code, const std::string &errorMessage, std::optional<uint64_t> reserved) = 0;

  /**
   * Compare equality
   */
  virtual bool equals(const Command &another) const {
    return encodeToString() == another.encodeToString();
  }

  /**
   * if the tag provided
   * it will be added into logEntry's specialTag
   * @return
   */
  virtual std::optional<std::string> specialTag() const {
    return std::nullopt;
  }

  /**
   * return the dedupId for this command
   * the custody is still under the command, if command is alive, the string_view is alive
   */
  virtual std::string_view dedupId() const {
    return "";
  }

 protected:
  /**
   * Holds all the meta data-related info. MetaData can be changed after command is created.
   */
  CommandMetaData mMetaData;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_COMMAND_H_

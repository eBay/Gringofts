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

#ifndef SERVER_SRC_KV_ENGINE_MODEL_COMMAND_H_
#define SERVER_SRC_KV_ENGINE_MODEL_COMMAND_H_

#include <functional>

#include <infra/monitor/MonitorTypes.h>
#include <infra/util/TimeUtil.h>

#include "../../../protocols/generated/common.pb.h"
#include "../../../protocols/generated/service.pb.h"
#include "../utils/Status.h"
#include "../store/KVStore.h"
#include "../store/VersionStore.h"
#include "Event.h"

namespace goblin::kvengine::model {

/// TODO: refactor this class to be a template class
class CommandContext {
 public:
  void setCreateTimeInNanos() {
     mMetrics.mCommandCreateTimeInNanos = gringofts::TimeUtil::currentTimeInNanos();
  }

  void setCommandOutQueueTimeInNanos() {
     mMetrics.mCommandOutQueueTimeInNanos = gringofts::TimeUtil::currentTimeInNanos();
  }

  void setCommandLockTimeInNanos() {
     mMetrics.mCommandLockTimeInNanos = gringofts::TimeUtil::currentTimeInNanos();
  }

  void setCommandPreExecutedTimeInNanos() {
     mMetrics.mCommandPreExecutedTimeInNanos = gringofts::TimeUtil::currentTimeInNanos();
  }

  void setCommandExecutedTimeInNanos() {
     mMetrics.mCommandExecutedTimeInNanos = gringofts::TimeUtil::currentTimeInNanos();
  }

  void setBeforeRaftCommitTimeInNanos() {
     mMetrics.mBeforeRaftCommitTimeInNanos = gringofts::TimeUtil::currentTimeInNanos();
  }

  gringofts::TimestampInNanos getCreatedTimeInNanos() {
     return mMetrics.mCommandCreateTimeInNanos;
  }

  void reportMetrics() {
     /// metrics, time cost stay in worker's queue
     auto summaryInQueue = gringofts::getSummary("request_call_stay_in_queue_latency_in_ms", {});
     auto inQueueLatency = (this->mMetrics.mCommandOutQueueTimeInNanos -
         this->mMetrics.mCommandCreateTimeInNanos) / 1000000.0;
     summaryInQueue.observe(inQueueLatency);

     /// metrics, time cost till command locked
     auto summaryLocked = gringofts::getSummary("request_command_locked_latency_in_ms", {});
     auto lockedLatency = (this->mMetrics.mCommandLockTimeInNanos -
         this->mMetrics.mCommandCreateTimeInNanos) / 1000000.0;
     summaryLocked.observe(lockedLatency);

     /// metrics, time cost till command pre-executed
     auto summaryPreExecuted = gringofts::getSummary("request_command_preexecuted_latency_in_ms", {});
     auto preExecutedLatency = (this->mMetrics.mCommandPreExecutedTimeInNanos -
         this->mMetrics.mCommandCreateTimeInNanos) / 1000000.0;
     summaryPreExecuted.observe(preExecutedLatency);

     /// metrics, time cost till command executed
     auto summaryExecuted = gringofts::getSummary("request_command_executed_latency_in_ms", {});
     auto executedLatency = (this->mMetrics.mCommandExecutedTimeInNanos -
         this->mMetrics.mCommandCreateTimeInNanos) / 1000000.0;
     summaryExecuted.observe(executedLatency);

     /// metrics, time exclude commit
     auto summaryUncommit = gringofts::getSummary("request_call_without_commit_latency_in_ms", {});
     auto unCommitLatency = (this->mMetrics.mBeforeRaftCommitTimeInNanos -
         this->mMetrics.mCommandCreateTimeInNanos) / 1000000.0;
     summaryUncommit.observe(unCommitLatency);
     SPDLOG_INFO("debug: in queue {}, lock {}, preexecuted {}, executed {}, unCommit: {}",
         inQueueLatency, lockedLatency, preExecutedLatency, executedLatency, unCommitLatency);

     /// for put/get/delete/cas handler to report their metrics
     reportSubMetrics();
  }

  virtual void initSuccessResponse(
      const store::VersionType &curMaxVersion, const model::EventList &events) {
     /// by default, we don't do anything
  }
  virtual void fillResponseAndReply(
      proto::ResponseCode code, const std::string &message, std::optional<uint64_t> leaderId) {
     /// by default, we don't do anything
  }
  virtual std::set<store::KeyType> getTargetKeys() {
     return {};
  }
  virtual proto::RequestHeader getRequestHeader() {
     return proto::RequestHeader();
  }
  virtual bool skipPreExecuteCB() {
     /// by default, skip preExecuteCallBack
     return true;
  }

 protected:
  virtual void reportSubMetrics() {
     /// by default, we don't do anything
  }

  /// metrics
  struct Metrics {
     /// command create time in nanos
     gringofts::TimestampInNanos mCommandCreateTimeInNanos;

     /// pull-out of queue
     gringofts::TimestampInNanos mCommandOutQueueTimeInNanos;

     /// lock time
     gringofts::TimestampInNanos mCommandLockTimeInNanos;

     /// command pre-executed completed
     gringofts::TimestampInNanos mCommandPreExecutedTimeInNanos;

     /// command executed completed
     gringofts::TimestampInNanos mCommandExecutedTimeInNanos;

     /// before events to be commit to raft
     gringofts::TimestampInNanos mBeforeRaftCommitTimeInNanos;
  };

  Metrics mMetrics;
};

class Command {
 public:
  Command() = default;
  virtual ~Command() = default;

  virtual std::shared_ptr<CommandContext> getContext() {
     /// if no customized context, provide a default context that only report metrics
     return std::make_shared<CommandContext>();
  }

  /**
    * The CommandProcessor will invoke the following three interfaces in order
    */
  virtual utils::Status prepare(const std::shared_ptr<store::KVStore> &kvStore) = 0;

  virtual utils::Status execute(const std::shared_ptr<store::KVStore> &, EventList *) = 0;

  virtual utils::Status finish(const std::shared_ptr<store::KVStore> &kvStore, const EventList &events) = 0;
};

}  // namespace goblin::kvengine::model

#endif  // SERVER_SRC_KV_ENGINE_MODEL_COMMAND_H_

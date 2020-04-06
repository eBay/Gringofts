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

#ifndef SRC_APP_UTIL_COMMANDPROCESSLOOP_H_
#define SRC_APP_UTIL_COMMANDPROCESSLOOP_H_

#include <INIReader.h>
#include <spdlog/spdlog.h>

#include "../infra/common_types.h"
#include "../infra/es/Command.h"
#include "../infra/es/CommandEventStore.h"
#include "../infra/es/Loop.h"
#include "../infra/es/ProcessCommandStateMachine.h"
#include "../infra/es/ReadonlyCommandEventStore.h"
#include "../infra/es/Recoverable.h"
#include "../infra/monitor/MonitorTypes.h"

#include "CommandEventDecoderImpl.h"
#include "EventApplyLoop.h"

namespace gringofts {
namespace app {

/**
 * A loop which runs the business command process & event apply logic on app layer.
 * It runs on a dedicated thread.
 */
class CommandProcessLoopInterface : public Loop, public Recoverable {
 public:
  /**
   * Process command, generate events, persist both command and events
   * to event store and apply events to state machine (will mutate the state).
   *
   * @param command: the command to be processed
   */
  // @formatter:off
  virtual void processCommand(std::shared_ptr<Command> command) = 0;
  // @formatter:on
};

/**
 * @tparam StateMachineType:  type of state machine which
 *                            encapsulates app state and ops around the state
 */
template<typename StateMachineType>
class CommandProcessLoopBase : public CommandProcessLoopInterface {
 public:
  using EventApplyLoopPtr = std::shared_ptr<EventApplyLoopInterface>;
  // @formatter:off
  using CommandQueue = BlockingQueue<std::shared_ptr<Command>>;
  // @formatter:on

  CommandProcessLoopBase(const INIReader &reader,
                         const std::shared_ptr<CommandEventDecoder> &decoder,
                         DeploymentMode deploymentMode,
                         EventApplyLoopPtr eventApplyLoop,
                         CommandQueue &inputCommandQueue,  // NOLINT [runtime/references]
                         std::unique_ptr<ReadonlyCommandEventStore> readonlyCommandEventStore,
                         std::shared_ptr<CommandEventStore> commandEventStore,
                         const std::string &snapshotDir);

  ~CommandProcessLoopBase() override = default;

  /// The main function under standalone mode.
  void run() override;

  /// The main function under distributed mode.
  /// Do real business stuff iff current node is leader and has the latest state.
  void runDistributed() override;

  void shutdown() override { mShouldExit = true; }

 protected:
  /// distributed mode
  void onBecomeLeader(std::shared_ptr<Command> command);

  /// distributed mode
  /// return lastLogIndex of the term that leader SM (raft client) gets authority,
  /// which will be used as initial value of mLastCommandId.
  /// if leader is ready, everything is is fine.
  /// if leader steps down, there is no side effect.
  uint64_t waitTillLeaderIsReady() const {
    uint64_t currentTerm = mCommandEventStore->getCurrentTerm();
    return mEventApplyLoop->waitTillLeaderIsReadyOrStepDown(currentTerm);
  }

  /**
   * standalone mode
   * Replay all the events in event store after \p offset.
   *
   * @param offset the position after which events will be loaded and applied to state machine
   * @return id of the command processing of which generates the last applied event
   */
  std::optional<Id> replayEvents(std::optional<Id> offset);

  /// TODO: revert to private once ProcessCommandLoop in app_demo is removed
  std::atomic<bool> mShouldExit = false;
  /// true if recover() has been invoked.
  bool mRecovered = false;

  DeploymentMode mDeploymentMode;
  EventApplyLoopPtr mEventApplyLoop;

  std::unique_ptr<StateMachineType> mAppStateMachine;
  uint64_t mLastCommandId = 0;

  BlockingQueue<std::shared_ptr<Command>> &mInputCommandQueue;

  std::unique_ptr<ReadonlyCommandEventStore> mReadonlyCommandEventStore;
  std::shared_ptr<CommandEventStore> mCommandEventStore;

  std::shared_ptr<CommandEventDecoder> mCommandEventDecoder;
  CryptoUtil mCrypto;
  std::string mSnapshotDir;

  /// metrics
  santiago::MetricsCenter::CounterType processed_command_total;
  santiago::MetricsCenter::CounterType applied_event_total;
  santiago::MetricsCenter::GaugeType consumer_queue_size;
  santiago::MetricsCenter::GaugeType transition_gauge;
};

template<typename StateMachineType>
CommandProcessLoopBase<StateMachineType>::CommandProcessLoopBase(
    const INIReader &reader,
    const std::shared_ptr<CommandEventDecoder> &decoder,
    DeploymentMode deploymentMode,
    EventApplyLoopPtr eventApplyLoop,
    CommandQueue &inputCommandQueue,  // NOLINT [runtime/references]
    std::unique_ptr<ReadonlyCommandEventStore> readonlyCommandEventStore,
    std::shared_ptr<CommandEventStore> commandEventStore,
    const std::string &snapshotDir)
    : mDeploymentMode(deploymentMode),
      mCommandEventDecoder(decoder),
      mEventApplyLoop(eventApplyLoop),
      mInputCommandQueue(inputCommandQueue),
      mReadonlyCommandEventStore(std::move(readonlyCommandEventStore)),
      mCommandEventStore(commandEventStore),
      mSnapshotDir(snapshotDir),
      processed_command_total(getCounter("processed_command_total", {})),
      applied_event_total(getCounter("applied_event_total", {})),
      consumer_queue_size(getGauge("consumer_queue_size", {})),
      transition_gauge(getGauge("transition_gauge", {
          {"OldFollowerToNewFollower", std::to_string(static_cast<double>(Transition::OldFollowerToNewFollower))},
          {"FollowerToLeader", std::to_string(static_cast<double>(Transition::FollowerToLeader))},
          {"LeaderToFollower", std::to_string(static_cast<double>(Transition::LeaderToFollower))},
          {"OldLeaderToNewLeader", std::to_string(static_cast<double>(Transition::OldLeaderToNewLeader))},
          {"SameFollower", std::to_string(static_cast<double>(Transition::SameFollower))},
          {"SameLeader", std::to_string(static_cast<double>(Transition::SameLeader))},
      })) {
  assert(mEventApplyLoop);

  mCrypto.init(reader);
  mAppStateMachine = std::make_unique<StateMachineType>();
}

template<typename StateMachineType>
void CommandProcessLoopBase<StateMachineType>::runDistributed() {
  assert(mDeploymentMode == DeploymentMode::Distributed);

  while (!mShouldExit) {
    try {
      auto command = mInputCommandQueue.dequeue();
      Transition transition = mCommandEventStore->detectTransition();

      /// monitoring
      transition_gauge.set(static_cast<double>(transition));

      /// Prepare & Accept
      if (transition == Transition::OldLeaderToNewLeader
          || transition == Transition::FollowerToLeader) {
        onBecomeLeader(command);
      }

      /// Reject
      if (transition == Transition::LeaderToFollower
          || transition == Transition::OldFollowerToNewFollower
          || transition == Transition::SameFollower) {
        command->onPersistFailed("Not a leader any longer", std::nullopt);
      }

      /// Accept
      if (transition == Transition::SameLeader) {
        processCommand(command);
      }
    } catch (const QueueStoppedException &e) {
      SPDLOG_WARN("input command queue has been closed: {}", e.what());
      shutdown();
    }
  }
}

template<typename StateMachineType>
void CommandProcessLoopBase<StateMachineType>::onBecomeLeader(std::shared_ptr<Command> command) {
  SPDLOG_INFO("Waiting for EventApplyLoop to catch up.");

  /// Wait
  uint64_t ts1InNano = TimeUtil::currentTimeInNanos();
  auto ret = waitTillLeaderIsReady();

  /// Reject if leader step down
  if (ret == 0) {
    SPDLOG_WARN("Leader step down during EventApplyLoop catching up.");
    command->onPersistFailed("Not a leader any longer", std::nullopt);
    return;
  }

  /// Reset command id.
  mLastCommandId = ret;

  /// swapState, teardown, set flag
  uint64_t ts2InNano = TimeUtil::currentTimeInNanos();
  mEventApplyLoop->swapStateAndTeardown(*mAppStateMachine);

  uint64_t ts3InNano = TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("onBecomeLeader Succeed, "
              "waitTillLeaderIsReady cost {}ms, swapStateAndTeardown cost {}ms. "
              "Processing new command",
              (ts2InNano - ts1InNano) / 1000000.0,
              (ts3InNano - ts2InNano) / 1000000.0);

  /// Start processing command
  processCommand(command);
}

template<typename StateMachineType>
void CommandProcessLoopBase<StateMachineType>::run() {
  assert(mDeploymentMode == DeploymentMode::Standalone);

  while (!mShouldExit) {
    SPDLOG_INFO("listening to new command");
    try {
      const auto &command = mInputCommandQueue.dequeue();
      consumer_queue_size.set(mInputCommandQueue.size());

      processCommand(std::move(command));
    }
    catch (const QueueStoppedException &e) {
      SPDLOG_WARN("input command queue has been closed: {}", e.what());
      shutdown();
    }
  }
}

template<typename StateMachineType>
std::optional<Id> CommandProcessLoopBase<StateMachineType>::replayEvents(std::optional<Id> offset) {
  assert(mDeploymentMode == DeploymentMode::Standalone);

  if (offset) {
    mReadonlyCommandEventStore->setCurrentOffset(*offset);
  }
  SPDLOG_INFO("Will start replaying events after offset {}", offset.value_or(0));
  int eventCount = 0;
  Id currentCommandId;
  auto commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandEventDecoder,
                                                                            *mCommandEventDecoder);
  while (commandEventsOpt) {
    currentCommandId = commandEventsOpt.value().first->getId();
    auto &events = commandEventsOpt.value().second;

    for (auto &eventPtr : events) {
      eventCount += 1;
      mAppStateMachine->applyEvent(*eventPtr);
      applied_event_total.increase();
    }

    commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandEventDecoder,
                                                                         *mCommandEventDecoder);
  }

  SPDLOG_INFO("Replay completed, total applied {} events", eventCount);
  return eventCount == 0 ? std::nullopt : std::optional<Id>(currentCommandId);
}

template<typename StateMachineType, bool = IsRocksDBBacked<StateMachineType>::value>
class CommandProcessLoop;  /// not defined

template<typename MemoryBackedStateMachineType>
class CommandProcessLoop<MemoryBackedStateMachineType, false>
    : public CommandProcessLoopBase<MemoryBackedStateMachineType> {
 public:
  using CommandProcessLoopBase<MemoryBackedStateMachineType>::CommandProcessLoopBase;

  /// standalone mode
  void recoverOnce() override {
    assert(this->mDeploymentMode == DeploymentMode::Standalone);

    if (this->mRecovered) {
      throw std::runtime_error("Error: Application has already been recovered, calling it multiple times will make "
                               "application state inconsistent. Exit...");
    }

    this->mRecovered = true;
    std::optional<Id> offset = SnapshotUtil::loadLatestSnapshot(this->mSnapshotDir,
                                                                *this->mAppStateMachine,
                                                                *this->mCommandEventDecoder,
                                                                *this->mCommandEventDecoder,
                                                                this->mCrypto);
    this->replayEvents(offset);
  }
};

template<typename RocksDBBackedStateMachineType>
class CommandProcessLoop<RocksDBBackedStateMachineType, true>
    : public CommandProcessLoopBase<RocksDBBackedStateMachineType> {
 public:
  using CommandProcessLoopBase<RocksDBBackedStateMachineType>::CommandProcessLoopBase;

  /// standalone mode
  void recoverOnce() override {
    assert(this->mDeploymentMode == DeploymentMode::Standalone);
    throw std::runtime_error("Error: RocksDBBacked AppStateMachine does not "
                             "support standalone mode. Exit...");
  }
};

}  /// namespace app
}  /// namespace gringofts

#endif  // SRC_APP_UTIL_COMMANDPROCESSLOOP_H_

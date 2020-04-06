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

#ifndef SRC_APP_UTIL_EVENTAPPLYLOOP_H_
#define SRC_APP_UTIL_EVENTAPPLYLOOP_H_

#include <INIReader.h>

#include "../infra/es/Loop.h"
#include "../infra/es/ReadonlyCommandEventStore.h"
#include "../infra/es/StateMachine.h"
#include "../infra/es/store/SnapshotUtil.h"
#include "../infra/util/CryptoUtil.h"

#include "CommandEventDecoderImpl.h"
#include "NetAdminServiceProvider.h"

namespace gringofts {
namespace app {

/**
 * A loop which subscribes to event store and applies events to app's state machine.
 */
class EventApplyLoopInterface : public Loop,
                                public NetAdminServiceProvider {
 public:
  /**
   * Interaction between CPL and EAL.
   * EAL, a.k.a, EventApplyLoop
   * CPL, a.k.a, CommandProcessLoop
   */

  /**
   * A lock-free function called by CPL to wait for state machine of EAL to
   * catch up on expectedTerm, or exit if raft leader of expectedTerm steps down.
   *
   * @param expectedTerm: the term being waited on
   * @return lastLogIndex if leader is ready, otherwise 0 if leader has stepped down
   */
  virtual uint64_t waitTillLeaderIsReadyOrStepDown(uint64_t expectedTerm) const = 0;

  /**
   * Called by CPL thread to
   * 1) swap state machine with CPL,
   * 2) teardown Readonly CES,
   * 3) set shouldRecover flag.
   *
   * Invoked when CPL becomes leader, and state machine of EAL is ready.
   * @param target: state machine of CPL
   */
  virtual void swapStateAndTeardown(StateMachine &target) = 0;  // NOLINT [runtime/references]

  /**
   * For unit-test only
   */
  virtual void replayTillNoEvent() = 0;
  virtual const StateMachine &getStateMachine() const = 0;
};

/**
 * A loop which subscribes to event store and applies events to app's state machine.
 * The type of state machine is same as the one in #gringofts::app:CommandProcessLoop.
 *
 * @tparam StateMachineType   : type of state machine that encapsulates
 *                              app state and ops around the state
 */
template<typename StateMachineType>
class EventApplyLoopBase : public EventApplyLoopInterface {
 public:
  EventApplyLoopBase(const INIReader &reader,
                     const std::shared_ptr<CommandEventDecoder> &decoder,
                     std::unique_ptr<ReadonlyCommandEventStore> readonlyCommandEventStore,
                     const std::string &snapshotDir)
      : mReadonlyCommandEventStore(std::move(readonlyCommandEventStore)),
        mCommandEventDecoder(decoder),
        mSnapshotDir(snapshotDir),
        mLastAppliedIndexGauge(getGauge("eal_last_applied_index", {})) { mCrypto.init(reader); }

  ~EventApplyLoopBase() override = default;

  /**
   * implement Loop
   */
  void run() override;
  void shutdown() override { mShouldExit = true; }

  /**
   * implement NetAdminServiceProvider
   */
  void truncatePrefix(uint64_t offsetKept) override {
    mReadonlyCommandEventStore->truncatePrefix(offsetKept);
  }

  /**
   * implement EventApplyLoopInterface
   */
  uint64_t waitTillLeaderIsReadyOrStepDown(uint64_t expectedTerm) const override {
    return mReadonlyCommandEventStore->waitTillLeaderIsReadyOrStepDown(expectedTerm);
  }

  void swapStateAndTeardown(StateMachine &target) override {  // NOLINT [runtime/references]
    std::lock_guard<std::mutex> lock(mLoopMutex);
    target.swapState(mAppStateMachine.get());
    mReadonlyCommandEventStore->teardown();
    mShouldRecover = true;
  }

  /**
   * For unit-test only
   */
  void replayTillNoEvent() override;

  const StateMachine &getStateMachine() const override {
    SPDLOG_WARN("should ONLY see this log in unit test");
    return *mAppStateMachine;
  }

 protected:
  /**
   * recover StateMachine
   */
  virtual void recoverSelf() = 0;

  std::unique_ptr<ReadonlyCommandEventStore> mReadonlyCommandEventStore;
  std::shared_ptr<CommandEventDecoder> mCommandEventDecoder;

  mutable CryptoUtil mCrypto;
  std::string mSnapshotDir;

  std::atomic<bool> mShouldExit = false;

  /**
   * mShouldRecover, mAppStateMachine, mLastAppliedLogEntryIndex
   * is protected by mLoopMutex.
   */
  mutable std::mutex mLoopMutex;

  /// should not use mAppStateMachine and mLastAppliedLogEntryIndex
  /// when mShouldRecover is true.
  std::atomic<bool> mShouldRecover = false;

  std::unique_ptr<StateMachineType> mAppStateMachine;
  uint64_t mLastAppliedLogEntryIndex = 0;

  /// metrics
  santiago::MetricsCenter::GaugeType mLastAppliedIndexGauge;
};

/**
 * Called by EAL thread
 */
template<typename StateMachineType>
void EventApplyLoopBase<StateMachineType>::run() {
  while (!mShouldExit) {
    std::lock_guard<std::mutex> lock(mLoopMutex);

    if (mShouldRecover) {
      recoverSelf();
    }

    uint64_t ts1InNano = TimeUtil::currentTimeInNanos();
    auto commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandEventDecoder,
                                                                              *mCommandEventDecoder);
    if (!commandEventsOpt) {
      continue;
    }

    uint64_t ts2InNano = TimeUtil::currentTimeInNanos();

    auto &events = (*commandEventsOpt).second;
    for (auto &eventPtr : events) {
      mAppStateMachine->applyEvent(*eventPtr);
    }

    uint64_t ts3InNano = TimeUtil::currentTimeInNanos();

    auto commandId = commandEventsOpt.value().first->getId();
    mAppStateMachine->commit(commandId);

    mLastAppliedLogEntryIndex = commandId;
    mLastAppliedIndexGauge.set(mLastAppliedLogEntryIndex);

    /// TODO: remove below if-block once log cutoff is supported
    if (commandId % 10 == 0) {
      SPDLOG_INFO("Apply Events, CommandId={}, EventNum={}, "
                  "totalCost={}us, loadCost={}us, applyCost={}us",
                  commandId,
                  events.size(),
                  (ts3InNano - ts1InNano) / 1000.0,
                  (ts2InNano - ts1InNano) / 1000.0,
                  (ts3InNano - ts2InNano) / 1000.0);
    }
  }
}

template<typename StateMachineType>
void EventApplyLoopBase<StateMachineType>::replayTillNoEvent() {
  SPDLOG_WARN("should ONLY see this log in unit test");
  std::unique_ptr<Event> eventPtr = mReadonlyCommandEventStore->loadNextEvent(*mCommandEventDecoder);
  while (eventPtr) {
    SPDLOG_INFO("Got Event, CommandId={}, EventId={}", eventPtr->getCommandId(), eventPtr->getId());
    std::lock_guard<std::mutex> lock(mLoopMutex);
    mAppStateMachine->applyEvent(*eventPtr);
    eventPtr = mReadonlyCommandEventStore->loadNextEvent(*mCommandEventDecoder);
  }
  mAppStateMachine->generateMockData();
}

/**
 * use SFINAE(https://en.cppreference.com/w/cpp/language/sfinae)
 * to determine whether the state machine is backed by RocksDB
 * via checking if it defines type RocksDBConf
 */
template<typename T>
struct IsRocksDBBackedHelper {
  template<typename Tp, typename = typename Tp::RocksDBConf>
  static std::true_type __test(int);

  template<typename>
  static std::false_type __test(...);

  typedef decltype(__test<T>(0)) type;
};

template<typename T>
struct IsRocksDBBacked : public IsRocksDBBackedHelper<T>::type {};

template<typename StateMachineType, bool = IsRocksDBBacked<StateMachineType>::value>
class EventApplyLoop;  /// not defined

template<typename MemoryBackedStateMachineType>
class EventApplyLoop<MemoryBackedStateMachineType, false>
        : public EventApplyLoopBase<MemoryBackedStateMachineType> {
 public:
  EventApplyLoop(const INIReader &reader,
                 const std::shared_ptr<CommandEventDecoder> &decoder,
                 std::unique_ptr<ReadonlyCommandEventStore> readonlyCommandEventStore,
                 const std::string &snapshotDir)
         : EventApplyLoopBase<MemoryBackedStateMachineType>(reader,
                                                            decoder,
                                                            std::move(readonlyCommandEventStore),
                                                            snapshotDir) {
    initStateMachine(reader);
    /// recover state
    recoverSelf();
  }

  /**
   * Called by NetAdminServer thread triggered by PuBuddy
   */
  std::pair<bool, std::string> takeSnapshotAndPersist() const override {
    /// TODO: takeSnapshotAndPersist() may hurt waitTillLeaderIsReady()
    std::lock_guard<std::mutex> lock(this->mLoopMutex);

    if (this->mShouldRecover) {
      SPDLOG_WARN("Defer taking snapshot during recover.");
      return std::make_pair(false, "");
    }

    auto offset = this->mLastAppliedLogEntryIndex;
    return SnapshotUtil::takeSnapshotAndPersist(offset,
                                                this->mSnapshotDir,
                                                *this->mAppStateMachine,
                                                this->mCrypto);
  }

  std::optional<uint64_t> getLatestSnapshotOffset() const override {
    return SnapshotUtil::findLatestSnapshotOffset(this->mSnapshotDir);
  }

 protected:
  void initStateMachine(const INIReader &) {
    this->mAppStateMachine = std::make_unique<MemoryBackedStateMachineType>();
  }

  void recoverSelf() override {
    SPDLOG_INFO("Start recovering.");

    /// clear state machine
    auto ts1InNano = TimeUtil::currentTimeInNanos();
    this->mAppStateMachine->clearState();

    /// recover state machine from snapshot
    auto ts2InNano = TimeUtil::currentTimeInNanos();
    auto readOffsetOpt = SnapshotUtil::loadLatestSnapshot(this->mSnapshotDir,
                                                          *this->mAppStateMachine,
                                                          *this->mCommandEventDecoder,
                                                          *this->mCommandEventDecoder,
                                                          this->mCrypto);
    /// recover applied index
    auto ts3InNano = TimeUtil::currentTimeInNanos();
    if (readOffsetOpt) {
      this->mLastAppliedLogEntryIndex = *readOffsetOpt;
    }

    /// re-init Readonly CES, unit test CAN ignore this step.
    if (this->mReadonlyCommandEventStore) {
      if (readOffsetOpt) {
        this->mReadonlyCommandEventStore->setCurrentOffset(*readOffsetOpt);
      }
      this->mReadonlyCommandEventStore->init();
    }

    auto ts4InNano = TimeUtil::currentTimeInNanos();
    SPDLOG_INFO("clear state cost {}ms, reload snapshot cost {}ms, "
                "re-init Readonly CES cost {}ms, will start apply events after {}",
                (ts2InNano - ts1InNano) / 1000000.0,
                (ts3InNano - ts2InNano) / 1000000.0,
                (ts4InNano - ts3InNano) / 1000000.0,
                readOffsetOpt.value_or(0));

    this->mShouldRecover = false;
  }
};

template<typename RocksDBBackedStateMachineType>
class EventApplyLoop<RocksDBBackedStateMachineType, true>
        : public EventApplyLoopBase<RocksDBBackedStateMachineType> {
 public:
  EventApplyLoop(const INIReader &reader,
                 const std::shared_ptr<CommandEventDecoder> &decoder,
                 std::unique_ptr<ReadonlyCommandEventStore> readonlyCommandEventStore,
                 const std::string &snapshotDir)
          : EventApplyLoopBase<RocksDBBackedStateMachineType>(reader,
                                                              decoder,
                                                              std::move(readonlyCommandEventStore),
                                                              snapshotDir) {
    initStateMachine(reader);
    /// recover state
    recoverSelf();
  }

  std::pair<bool, std::string> takeSnapshotAndPersist() const override {
    /// create Checkpoint of RocksDB is thread-safe,
    /// we don't need lock mLoopMutex.
    auto checkpointPath = this->mAppStateMachine->createCheckpoint(this->mSnapshotDir);
    return std::make_pair(true, checkpointPath);
  }

  std::optional<uint64_t> getLatestSnapshotOffset() const override {
    /// RocksDBBacked StateMachine use checkpoint instead of snapshot
    return SnapshotUtil::findLatestCheckpointOffset(this->mSnapshotDir);
  }

 protected:
  void initStateMachine(const INIReader &iniReader) {
    std::string walDir = iniReader.Get("rocksdb", "wal.dir", "");
    std::string dbDir  = iniReader.Get("rocksdb", "db.dir", "");
    assert(!walDir.empty() && !dbDir.empty());

    this->mAppStateMachine = std::make_unique<RocksDBBackedStateMachineType>(walDir, dbDir);
  }

  void recoverSelf() override {
    SPDLOG_INFO("Start recovering.");

    /// recover StateMachine
    this->mLastAppliedLogEntryIndex = this->mAppStateMachine->recoverSelf();

    /// re-init Readonly CES, unit test CAN ignore this step.
    if (this->mReadonlyCommandEventStore) {
      this->mReadonlyCommandEventStore->setCurrentOffset(this->mLastAppliedLogEntryIndex);
      this->mReadonlyCommandEventStore->init();
    }

    this->mShouldRecover = false;
  }
};

}  /// namespace app
}  /// namespace gringofts

#endif  // SRC_APP_UTIL_EVENTAPPLYLOOP_H_

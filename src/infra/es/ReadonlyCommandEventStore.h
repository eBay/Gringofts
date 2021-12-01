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

#ifndef SRC_INFRA_ES_READONLYCOMMANDEVENTSTORE_H_
#define SRC_INFRA_ES_READONLYCOMMANDEVENTSTORE_H_

#include "../common_types.h"
#include "Command.h"
#include "CommandDecoder.h"
#include "Event.h"
#include "EventDecoder.h"

namespace gringofts {

/**
 * Readonly command event store which only reads commands and events from the store.
 */
class ReadonlyCommandEventStore {
 public:
  ReadonlyCommandEventStore() = default;
  virtual ~ReadonlyCommandEventStore() = default;

  /// forbidden copy and move
  ReadonlyCommandEventStore(const ReadonlyCommandEventStore &) = delete;
  ReadonlyCommandEventStore &operator=(const ReadonlyCommandEventStore &) = delete;

  /**
   * An interface for two phase initialize.
   */
  virtual void init() {}

  /**
   * An interface for cleanup internal state
   */
  virtual void teardown() {}

  /**
   * Load the next event from the event store, decode it into an ::gringofts::Event instance and return
   *
   * A typical use scenario is app recovery, where app will load and replay all the events persisted in the
   * event store.
   *
   * @param eventDecoder used to transform from the information stored in event store to
   * an in-memory ::gringofts::Event instance
   * @return an unique pointer to an in-memory ::gringofts::Event instance. The unique pointer will be
   * empty if all the events are loaded
   */
  virtual std::unique_ptr<Event> loadNextEvent(const EventDecoder &eventDecoder) = 0;

  /**
   * Load the next command after the one whose id is \p prevCommandId from the command store,
   * decode it into a ::gringofts::Command instance and return.
   *
   * The command should not have been processed yet, in other words, there's no event in the event store
   * whose commandId refers to this command.
   *
   * A typical use scenario is app recovery, where after app has done replaying all the events, it starts processing
   * all the un-processed commands persisted in the command store.
   *
   * @param prevCommandId The id of the previous ::gringofts::Command, events of which have been persisted in the
   * command store
   * @param commandDecoder used to transform from the information stored in command store
   * to an in-memory ::gringofts::Command instance
   * @return an unique pointer to an in-memory ::gringofts::Command instance. The unique pointer will be
   * empty if all the commands are loaded
   */
  virtual std::unique_ptr<Command> loadCommandAfter(Id prevCommandId,
                                                    const CommandDecoder &commandDecoder) = 0;

  /**
   * Load the next command from the command store, decode it into a ::gringofts::Command instance and return.
   *
   * The command should not have been processed yet, in other words, there's no event in the event store
   * whose commandId refers to this command.
   *
   * A typical use scenario is app recovery, where there's no event for the app to replay.
   *
   * @param commandDecoder used to transform from the information stored in command store
   * to an in-memory ::gringofts::Command instance
   * @return an unique pointer to an in-memory ::gringofts::Command instance. The unique pointer will be
   * empty if all the commands are loaded
   */
  virtual std::unique_ptr<Command> loadNextCommand(const CommandDecoder &commandDecoder) = 0;

  /**
   * Load the next command and corresponding events from the command event store,
   * decode it into a std::pair<std::unique_ptr<Command>, std::queue<std::unique_ptr<Event>>> and return.
   *
   * A typical use scenario is for post engine.
   *
   * @param commandDecoder used to transform from the information stored in command store
   * to an in-memory ::gringofts::Command instance
   * @param eventDecoder used to transform from the information stored in event store
   * to an in-memory ::gringofts::Event instance
   * @return an optional to a pair of command and events. The optional will be *nullopt*
   * if no command and events are available.
   */
  using CommandEvents = std::pair<std::unique_ptr<Command>, std::list<std::unique_ptr<Event>>>;
  using CommandEventsOpt = std::optional<CommandEvents>;
  using CommandEventsList = std::list<CommandEvents>;

  /**
   * app layer must apply all the previous events before invoking this method.
   * Sample usage (quoted from EventApplyLoop::run()):
   *
   * ```
   * while (true) {
   *   auto commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(mCommandEventDecoder,
   *                                                                             mCommandEventDecoder);
   *   if (!commandEventsOpt) {
   *     continue;
   *   }
   *
   *   auto &events = (*commandEventsOpt).second;
   *
   *   for (auto &eventPtr : events) {
   *     mAppStateMachine->applyEvent(*eventPtr);
   *   }
   *
   *   auto commandId = (*commandEventsOpt).first->getId();
   *   mLastAppliedLogEntryIndex = commandId;
   * }
   * ```
   */
  virtual CommandEventsOpt loadNextCommandEvents(const CommandDecoder &commandDecoder,
                                                 const EventDecoder &eventDecoder) = 0;

  virtual uint64_t loadCommandEventsList(const CommandDecoder &commandDecoder,
                                         const EventDecoder &eventDecoder,
                                         Id commandId,
                                         uint64_t size,
                                         CommandEventsList *bundles) = 0;

  /**
   * wait till leader is ready for expected term or step down,
   * so that the app layer can start to serve new request.
   *
   * This method is usually called by the app layer in the thread (e.g., command process loop) than the
   * owning thread (e.g., event apply loop).
   *
   * @return lastLogIndex of expected term.
   */
  virtual uint64_t waitTillLeaderIsReadyOrStepDown(uint64_t expectedTerm) const {
    throw std::runtime_error("Not supported");
  }
  /*
   * @return true if currently this node is the leader
   */
  virtual bool isLeader() const {
    throw std::runtime_error("Not supported");
  }

  /**
   * Get the current read offset
   * For raft-backed store, it is #gringofts::ReadonlyRaftCommandEventStore::mAppliedIndex.
   * For sqlite-backed store, it is #gringofts::ReadonlySQLiteCommandEventStore::mCurrentLoadedEventOffset.
   */
  virtual uint64_t getCurrentOffset() const = 0;

  /**
   * Set the current read offset
   * For raft-backed store, it will set #gringofts::ReadonlyRaftCommandEventStore::mAppliedIndex.
   * For sqlite-backed store, it will set #gringofts::ReadonlySQLiteCommandEventStore::mCurrentLoadedEventOffset.
   * @param currentOffset the read offset usually sourced from snapshot
   */
  virtual void setCurrentOffset(uint64_t currentOffset) = 0;

  /**
   * Discard entries before offsetKept.
   * For log retention purpose, entries that already backup can be discarded.
   */
  virtual void truncatePrefix(uint64_t offsetKept) = 0;

  /**
   * Is syncing log
   */
  virtual bool isSyncing() const {
    return false;
  }

  /**
   * the first index we can fetch data
   */
  virtual uint64_t firstIndex() const {
    return 0;
  }

  /**
   * The begin index of this cluster
   */
  virtual uint64_t beginIndex() const {
    return 0;
  }
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_READONLYCOMMANDEVENTSTORE_H_

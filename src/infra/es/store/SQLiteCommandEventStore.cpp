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

#include "SQLiteCommandEventStore.h"

#include <assert.h>
#include <vector>

#include <spdlog/spdlog.h>

namespace gringofts {

SQLiteCommandEventStore::SQLiteCommandEventStore(std::shared_ptr<SQLiteStoreDao> dao,
                                                 std::shared_ptr<IdGenerator> commandIdGenerator,
                                                 std::shared_ptr<IdGenerator> eventIdGenerator) :
    mSqliteStoreDao(dao),
    mCommandIdGenerator(commandIdGenerator),
    mEventIdGenerator(eventIdGenerator) {
  assert(mCommandIdGenerator != nullptr);
  assert(mEventIdGenerator != nullptr);
}

void SQLiteCommandEventStore::persist(
    const std::vector<std::shared_ptr<Command>> &commands,
    const std::vector<std::shared_ptr<Event>> &events) {
  mSqliteStoreDao->persist(commands, events);
}

void SQLiteCommandEventStore::persistAsync(const std::shared_ptr<Command> &command,
                                           const std::vector<std::shared_ptr<Event>> &events,
                                           uint64_t code,
                                           const std::string &message) {
  /// command with no events should skip persist.
  if (events.empty()) {
    command->onPersisted(message.c_str());
    return;
  }

  CommandEventsEntry commandEventsEntry(command, events);
  try {
    Id commandId = mCommandIdGenerator->getNextId();
    command->setId(commandId);
    for (const auto &event : events) {
      event->setCommandId(commandId);
      Id eventId = mEventIdGenerator->getNextId();
      event->setId(eventId);
    }
    mCommandEventQueue.enqueue(std::move(commandEventsEntry));
  } catch (const QueueStoppedException &e) {
    SPDLOG_WARN("Command event queue stopped: {}", e.what());
  }
}

void SQLiteCommandEventStore::run() {
  if (mShouldExit) {
    SPDLOG_WARN("The store is already down. Will not run again.");
    return;
  }

  if (mStarted) {
    SPDLOG_WARN("Already started, ignored this call.");
    return;
  }

  mStarted = true;
  while (!mShouldExit) {
    SPDLOG_WARN("Listening to new commandEventEntry");
    std::vector<std::shared_ptr<Command>> allCommands;
    std::vector<std::shared_ptr<Event>> allEvents;

    auto captureCommandAndEvents = [this, &allCommands, &allEvents]() {
      std::shared_ptr<Command> command;
      std::vector<std::shared_ptr<Event>> events;
      try {
        const CommandEventsEntry &commandEventsEntry = mCommandEventQueue.dequeue();
        tie(command, events) = commandEventsEntry;
        allCommands.emplace_back(command);
        allEvents.insert(allEvents.end(), events.begin(), events.end());
      } catch (const QueueStoppedException &e) {
        SPDLOG_WARN("Command event queue stopped: {}", e.what());
        shutdown();
      }
    };

    captureCommandAndEvents();

    // must ensure it's not negative as the restTotal is unsigned long, -1 is a very large number!
    if (mCommandEventQueue.size() > 0) {
      uint64_t restTotal = mCommandEventQueue.size();
      SPDLOG_INFO("Queue size={}", restTotal);
      for (uint64_t i = 0; i < restTotal; i++) {
        captureCommandAndEvents();
      }
    }

    /// it might already be down
    if (!mShouldExit) {
      SPDLOG_INFO("Persisting {} events", allEvents.size());
      persist(allCommands, allEvents);
    }
  }
}

void SQLiteCommandEventStore::shutdown() {
  mShouldExit = true;
  mStarted = false;
  mCommandEventQueue.shutdown();
}

}  /// namespace gringofts

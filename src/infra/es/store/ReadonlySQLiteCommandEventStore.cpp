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

#include "ReadonlySQLiteCommandEventStore.h"

#include <assert.h>

namespace gringofts {

ReadonlySQLiteCommandEventStore::ReadonlySQLiteCommandEventStore(std::shared_ptr<SQLiteStoreDao> dao,
                                                                 bool needUpdateId,
                                                                 std::shared_ptr<IdGenerator> commandIdGenerator,
                                                                 std::shared_ptr<IdGenerator> eventIdGenerator) :
    mSqliteStoreDao(dao),
    mNeedUpdateId(needUpdateId),
    mCommandIdGenerator(commandIdGenerator),
    mEventIdGenerator(eventIdGenerator) {
  if (mNeedUpdateId) {
    assert(mCommandIdGenerator != nullptr);
    assert(mEventIdGenerator != nullptr);
  }
}

std::unique_ptr<Event> ReadonlySQLiteCommandEventStore::loadNextEvent(const EventDecoder &eventDecoder) {
  auto ptr = mSqliteStoreDao->findNextEvent(mCurrentLoadedEventOffset, eventDecoder);
  if (ptr) {
    mCurrentLoadedEventOffset = ptr->getId();
    if (mNeedUpdateId) {
      mEventIdGenerator->setCurrentId(ptr->getId());
      mCommandIdGenerator->setCurrentId(ptr->getCommandId());
    }
  }
  return ptr;
}

std::unique_ptr<Command> ReadonlySQLiteCommandEventStore::loadNextCommand(const CommandDecoder &commandDecoder) {
  auto ptr = mSqliteStoreDao->findNextCommand(mCurrentLoadedCommandOffset, commandDecoder);
  if (ptr) {
    mCurrentLoadedCommandOffset = ptr->getId();
    if (mNeedUpdateId) {
      mCommandIdGenerator->setCurrentId(ptr->getId());
    }
  }
  return ptr;
}

std::unique_ptr<Command> ReadonlySQLiteCommandEventStore::loadCommandAfter(Id commandId,
                                                                           const CommandDecoder &commandDecoder) {
  mCurrentLoadedCommandOffset = commandId;
  return loadNextCommand(commandDecoder);
}

ReadonlySQLiteCommandEventStore::CommandEventsOpt
ReadonlySQLiteCommandEventStore::loadNextCommandEvents(const CommandDecoder &commandDecoder,
                                                       const EventDecoder &eventDecoder) {
  auto command = mSqliteStoreDao->findNextCommand(mCurrentLoadedCommandOffset, commandDecoder);
  if (!command) return std::nullopt;
  mCurrentLoadedCommandOffset = command->getId();
  if (mNeedUpdateId) {
    mCommandIdGenerator->setCurrentId(command->getId());
  }
  auto events = mSqliteStoreDao->getEventsByCommandId(mCurrentLoadedCommandOffset, eventDecoder);
  if (events.empty()) {
    SPDLOG_WARN("No events for command #{}", mCurrentLoadedCommandOffset);
  } else if (mNeedUpdateId) {
    mEventIdGenerator->setCurrentId(events.back()->getId());
  }

  return std::make_pair(std::move(command), std::move(events));
}

uint64_t
ReadonlySQLiteCommandEventStore::loadCommandEventsList(const gringofts::CommandDecoder &,
                                                       const gringofts::EventDecoder &,
                                                       Id, uint64_t, CommandEventsList *) {
  throw std::runtime_error("SQLite does not support loadCommandEventsList()");
}

}  /// namespace gringofts

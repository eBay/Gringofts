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

#include <gtest/gtest.h>

#include "../Dummies.h"
#include "../../../../src/infra/es/store/ReadonlySQLiteCommandEventStore.h"
#include "../../../../src/infra/es/store/SQLiteCommandEventStore.h"

namespace gringofts::test {

TEST(SQLiteCommandEventStoreTest, PersistAndLoadTest) {
  /// init
  Util::executeCmd("cp \"../test/infra/es/store/blank.db\" \"../test/infra/es/store/test.db\"");
  auto sqliteDao = std::make_shared<SQLiteStoreDao>("../test/infra/es/store/test.db");
  auto commandIdGenerator = std::make_shared<IdGenerator>();
  auto eventIdGenerator = std::make_shared<IdGenerator>();
  SQLiteCommandEventStore commandEventStore{sqliteDao, commandIdGenerator, eventIdGenerator};
  ReadonlySQLiteCommandEventStore readonlyCommandEventStore{sqliteDao, false, nullptr, nullptr};
  DummyCommandDecoder commandDecoder;
  DummyEventDecoder eventDecoder;

  auto command0 = DummyCommand::createDummyCommand();

  auto command1 = DummyCommand::createDummyCommand();
  auto events1 = DummyEvent::createDummyEvents(2);

  auto command2 = DummyCommand::createDummyCommand();
  auto events2 = DummyEvent::createDummyEvents(3);

  /// behavior
  auto persistThread = std::thread([&commandEventStore]() {
    commandEventStore.run();
  });
  EXPECT_NO_THROW(commandEventStore.persistAsync(std::move(command0), {}, 0, "test0"));
  EXPECT_NO_THROW(commandEventStore.persistAsync(std::move(command1), events1, 0, "test1"));
  EXPECT_NO_THROW(commandEventStore.persistAsync(std::move(command2), events2, 0, "test2"));

  ReadonlyCommandEventStore::CommandEventsOpt commandEventsOpt = std::nullopt;

  while (!commandEventsOpt) {
    commandEventsOpt = readonlyCommandEventStore.loadNextCommandEvents(commandDecoder, eventDecoder);
    sleep(1);
  }

  /// will not run
  EXPECT_NO_THROW(commandEventStore.run());

  commandEventStore.shutdown();
  if (persistThread.joinable()) {
    persistThread.join();
  }

  /// will not save
  auto command3 = DummyCommand::createDummyCommand();
  auto events3 = DummyEvent::createDummyEvents(3);
  EXPECT_NO_THROW(commandEventStore.persistAsync(std::move(command3), events3, 0, "test3"));
  /// will not run
  EXPECT_NO_THROW(commandEventStore.run());

  /// assert
  EXPECT_TRUE(commandEventsOpt);
  auto &loadedCommand = (*commandEventsOpt).first;
  auto &loadedEvents = (*commandEventsOpt).second;

  EXPECT_EQ(1, loadedCommand->getId());
  EXPECT_EQ(1, loadedCommand->getCreatorId());
  EXPECT_EQ(2, loadedCommand->getGroupId());
  EXPECT_EQ(3, loadedCommand->getGroupVersion());
  /// tracking context is not persisted in sqlite store
  EXPECT_EQ("", loadedCommand->getTrackingContext());
  EXPECT_EQ("DummyCommand", loadedCommand->encodeToString());
  EXPECT_EQ(nullptr, loadedCommand->getRequestHandle());

  EXPECT_EQ(2, loadedEvents.size());
  auto index = 1;
  for (const auto &eventPtr : loadedEvents) {
    EXPECT_EQ(index, eventPtr->getId());
    EXPECT_EQ(1, eventPtr->getCommandId());
    EXPECT_EQ(1, eventPtr->getCreatorId());
    EXPECT_EQ(2, eventPtr->getGroupId());
    EXPECT_EQ(3, eventPtr->getGroupVersion());
    EXPECT_EQ("DummyEvent", eventPtr->encodeToString());
    /// tracking context is not persisted in sqlite store
    EXPECT_EQ("", eventPtr->getTrackingContext());
    index += 1;
  }
}

TEST(ReadonlySQLiteCommandEventStoreTest, LoadWithNeedUpdateOnTest) {
  /// init
  Util::executeCmd("cp \"../test/infra/es/store/two-items.db\" \"../test/infra/es/store/test.db\"");
  auto sqliteDao = std::make_shared<SQLiteStoreDao>("../test/infra/es/store/test.db");
  auto commandIdGenerator = std::make_shared<IdGenerator>();
  auto eventIdGenerator = std::make_shared<IdGenerator>();
  ReadonlySQLiteCommandEventStore readonlyCommandEventStore{sqliteDao, true, commandIdGenerator, eventIdGenerator};
  DummyCommandDecoder commandDecoder;
  DummyEventDecoder eventDecoder;

  /// behavior
  auto firstCommand = readonlyCommandEventStore.loadCommandAfter(0, commandDecoder);

  /// assert
  EXPECT_EQ(1, readonlyCommandEventStore.getCurrentOffset());
  EXPECT_EQ(1, firstCommand->getId());

  /// behavior
  auto firstEvent = readonlyCommandEventStore.loadNextEvent(eventDecoder);
  auto secondEvent = readonlyCommandEventStore.loadNextEvent(eventDecoder);

  /// assert
  EXPECT_EQ(1, readonlyCommandEventStore.getCurrentOffset());
  EXPECT_EQ(1, firstEvent->getId());
  EXPECT_EQ(1, firstEvent->getCommandId());
  EXPECT_EQ(2, secondEvent->getId());
  EXPECT_EQ(1, secondEvent->getCommandId());

  /// behavior
  readonlyCommandEventStore.setCurrentOffset(1);
  auto secondCommand = readonlyCommandEventStore.loadNextCommand(commandDecoder);

  /// assert
  EXPECT_EQ(2, readonlyCommandEventStore.getCurrentOffset());
  EXPECT_EQ(2, secondCommand->getId());

  /// behavior
  auto thirdEvent = readonlyCommandEventStore.loadNextEvent(eventDecoder);

  /// assert
  EXPECT_EQ(2, readonlyCommandEventStore.getCurrentOffset());
  EXPECT_EQ(3, thirdEvent->getId());
  EXPECT_EQ(2, thirdEvent->getCommandId());

  /// more assertions
  EXPECT_THROW(readonlyCommandEventStore.loadCommandEventsList(commandDecoder, eventDecoder, 0, 0, nullptr),
               std::runtime_error);
  EXPECT_NO_THROW(readonlyCommandEventStore.truncatePrefix(0));
}

}  /// namespace gringofts::test

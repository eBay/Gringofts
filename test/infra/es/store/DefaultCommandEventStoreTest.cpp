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

#include "../../../../src/infra/es/store/DefaultCommandEventStore.h"
#include "../../../../src/infra/es/store/ReadonlyDefaultCommandEventStore.h"
#include "../Dummies.h"

namespace gringofts::test {

TEST(DefaultCommandEventStoreTest, Test) {
  /// init
  DefaultCommandEventStore commandEventStore{};
  auto timestamp = TimeUtil::currentTimeInNanos();
  const auto &command = std::make_shared<DummyCommand>(timestamp, "dummyCommand");
  std::vector<std::shared_ptr<Event>> events;
  events.push_back(std::make_shared<DummyEvent>(timestamp, "dummyEvent"));

  /// assert
  EXPECT_NO_THROW(commandEventStore.run());
  EXPECT_NO_THROW(commandEventStore.persistAsync(command, events, 0, "hello"));
  EXPECT_NO_THROW(commandEventStore.shutdown());

  EXPECT_THROW(commandEventStore.detectTransition(), std::runtime_error);
  EXPECT_THROW(commandEventStore.getCurrentTerm(), std::runtime_error);
}

TEST(ReadonlyDefaultCommandEventStoreTest, Test) {
  /// init
  ReadonlyDefaultCommandEventStore readonlyCommandEventStore{};
  DummyCommandDecoder commandDecoder;
  DummyEventDecoder eventDecoder;

  /// assert
  EXPECT_EQ(nullptr, readonlyCommandEventStore.loadNextEvent(eventDecoder));
  EXPECT_EQ(nullptr, readonlyCommandEventStore.loadCommandAfter(0, commandDecoder));
  EXPECT_EQ(nullptr, readonlyCommandEventStore.loadNextCommand(commandDecoder));
  EXPECT_EQ(std::nullopt, readonlyCommandEventStore.loadNextCommandEvents(commandDecoder, eventDecoder));
  EXPECT_EQ(0, readonlyCommandEventStore.loadCommandEventsList(commandDecoder, eventDecoder, 0, 0, nullptr));
  EXPECT_EQ(0, readonlyCommandEventStore.getCurrentOffset());
  EXPECT_NO_THROW(readonlyCommandEventStore.setCurrentOffset(1));
  EXPECT_NO_THROW(readonlyCommandEventStore.truncatePrefix(1));
}

}  /// namespace gringofts::test

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

#ifndef TEST_INFRA_ES_READONLYCOMMANDEVENTSTOREMOCK_H_
#define TEST_INFRA_ES_READONLYCOMMANDEVENTSTOREMOCK_H_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../../../src/infra/es/ReadonlyCommandEventStore.h"

namespace gringofts {

class ReadonlyCommandEventStoreMock : public ReadonlyCommandEventStore {
 public:
  ReadonlyCommandEventStoreMock() {}

  MOCK_METHOD0(init, void());
  MOCK_METHOD1(loadNextEvent, std::unique_ptr<Event>(
      const EventDecoder &eventDecoder));
  MOCK_METHOD1(loadNextCommand, std::unique_ptr<Command>(
      const CommandDecoder &commandDecoder));

  MOCK_METHOD2(loadCommandAfter, std::unique_ptr<Command>(Id
      prevCommandId,
      const CommandDecoder &commandDecoder));

  MOCK_METHOD2(loadNextCommandEvents, CommandEventsOpt(
      const CommandDecoder &commandDecoder,
      const EventDecoder &eventDecoder));

  MOCK_METHOD5(loadCommandEventsList, uint64_t(
      const CommandDecoder &commandDecoder,
      const EventDecoder &eventDecoder,
      Id commandId,
      uint64_t size,
      CommandEventsList * bundles));

  MOCK_CONST_METHOD0(getCurrentOffset, uint64_t());

  MOCK_METHOD1(setCurrentOffset, void(uint64_t));

  MOCK_METHOD1(truncatePrefix, void(uint64_t));
};

}  /// namespace gringofts

namespace testing {
// An implement of set arg pointee for ReadonlyCommandEventStore::CommandEventsList only.
template <size_t N>
class SetMovePointeeAction {
 public:
  // Constructs an action that sets the variable pointed to by the
  // N-th function argument to 'value'.
  explicit SetMovePointeeAction(gringofts::ReadonlyCommandEventStore::CommandEventsList* commandEventsList)
  : mCommandEventsList(commandEventsList) {}

  template <typename Result, typename ArgumentTuple>
  void Perform(const ArgumentTuple& args) const {
    internal::CompileAssertTypesEqual<void, Result>();
    gringofts::ReadonlyCommandEventStore::CommandEventsList& commandEventsList = *::testing::get<N>(args);
    commandEventsList = std::move(*mCommandEventsList);
  }

 private:
  mutable gringofts::ReadonlyCommandEventStore::CommandEventsList* mCommandEventsList;
};

// Creates an action that sets the variable pointed by the N-th
// (0-based) function argument to 'value'.
template <size_t N>
PolymorphicAction<SetMovePointeeAction<N> >
SetMovePointee(gringofts::ReadonlyCommandEventStore::CommandEventsList* commandEventsList) {
  return MakePolymorphicAction(SetMovePointeeAction<N>(commandEventsList));
}

}  /// namespace testing

#endif  // TEST_INFRA_ES_READONLYCOMMANDEVENTSTOREMOCK_H_

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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../../src/infra/es/CommandEventStore.h"

namespace gringofts::test {

class CommandEventStoreMock : public CommandEventStore {
  MOCK_METHOD0(run, void());
  MOCK_METHOD0(shutdown, void());
  MOCK_METHOD4(persistAsync, void(
      const std::shared_ptr<Command> &,
      const std::vector<std::shared_ptr<Event>> &,
      uint64_t,
      const std::string &));
};

TEST(CommandEventStoreTest, DefaultMethodsThrowRuntimeError) {
  /// init
  testing::NiceMock<CommandEventStoreMock> mock;

  /// assert
  EXPECT_THROW(mock.detectTransition(), std::runtime_error);
  EXPECT_THROW(mock.getCurrentTerm(), std::runtime_error);
}

}  /// namespace gringofts::test

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

#include "../../../../src/infra/es/CommandDecoder.h"
#include "../../../../src/infra/es/store/CommandEventDecodeWrapper.h"

namespace gringofts::test {
class MockCommandDecoder : public CommandDecoder {
 public:
  MOCK_CONST_METHOD2(decodeCommandFromString, std::unique_ptr<Command>(const CommandMetaData &, std::string_view));
};

class MockEventDecoder : public EventDecoder {
 public:
  MOCK_CONST_METHOD2(decodeEventFromString, std::unique_ptr<Event>(const EventMetaData &, std::string_view));
};

TEST(EventDecodeWrapperTest, SpecialCharacterCommandDecodeTest) {
  gringofts::es::CommandEntry commandEntry;
  commandEntry.set_entry("aaa\0bbb");
  MockCommandDecoder decoder;
  EXPECT_CALL(decoder, decodeCommandFromString(::testing::_, std::string_view("aaa\0bbb")));
  CommandEventDecodeWrapper::decodeCommand(commandEntry, decoder);
}

TEST(EventDecodeWrapperTest, SpecialCharacterEventDecodeTest) {
  gringofts::es::EventEntry eventEntry;
  eventEntry.set_entry("aaa\0bbb");
  MockEventDecoder decoder;
  EXPECT_CALL(decoder, decodeEventFromString(::testing::_, std::string_view("aaa\0bbb")));
  CommandEventDecodeWrapper::decodeEvent(eventEntry, decoder);
}
}  /// namespace gringofts::test

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

#include "../../../src/infra/es/CommandMetaData.h"
#include "../../../src/infra/es/EventMetaData.h"

namespace gringofts::test {

TEST(CommandEventMetaDataTest, CommandMetaData) {
  /// init
  auto timestamp = TimeUtil::currentTimeInNanos();
  es::CommandEntry commandEntry;
  commandEntry.set_id(1);
  commandEntry.set_creatorid(2);
  commandEntry.set_groupid(3);
  commandEntry.set_groupversion(4);
  commandEntry.set_trackingcontext("dummy tracking context");
  commandEntry.set_type(5);
  commandEntry.set_createdtimeinnanos(timestamp);
  CommandMetaData metaData{commandEntry};

  /// behavior
  es::CommandEntry commandEntry2;
  metaData.populateCommandEntry(&commandEntry2);

  /// assert
  EXPECT_EQ(1, commandEntry2.id());
  EXPECT_EQ(2, commandEntry2.creatorid());
  EXPECT_EQ(3, commandEntry2.groupid());
  EXPECT_EQ(4, commandEntry2.groupversion());
  EXPECT_EQ("dummy tracking context", commandEntry2.trackingcontext());
  EXPECT_EQ(5, commandEntry2.type());
  EXPECT_EQ(timestamp, commandEntry2.createdtimeinnanos());
}

TEST(CommandEventMetaDataTest, EventMetaData) {
  /// init
  auto timestamp = TimeUtil::currentTimeInNanos();
  es::EventEntry eventEntry;
  eventEntry.set_id(1);
  eventEntry.set_creatorid(2);
  eventEntry.set_groupid(3);
  eventEntry.set_groupversion(4);
  eventEntry.set_trackingcontext("dummy tracking context");
  eventEntry.set_type(5);
  eventEntry.set_createdtimeinnanos(timestamp);
  eventEntry.set_commandid(6);
  EventMetaData metaData{eventEntry};

  /// behavior
  es::EventEntry eventEntry2;
  metaData.populateEventEntry(&eventEntry2);

  /// assert
  EXPECT_EQ(1, eventEntry2.id());
  EXPECT_EQ(2, eventEntry2.creatorid());
  EXPECT_EQ(3, eventEntry2.groupid());
  EXPECT_EQ(4, eventEntry2.groupversion());
  EXPECT_EQ("dummy tracking context", eventEntry2.trackingcontext());
  EXPECT_EQ(5, eventEntry2.type());
  EXPECT_EQ(timestamp, eventEntry2.createdtimeinnanos());
  EXPECT_EQ(6, eventEntry2.commandid());
}

}  /// namespace gringofts::test

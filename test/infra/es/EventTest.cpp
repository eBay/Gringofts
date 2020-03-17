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

#include "Dummies.h"

namespace gringofts::test {

TEST(EventTest, MetaDataGetWhatSet) {
  /// init
  auto event = DummyEvent::createDummyEvent();

  EXPECT_EQ(0, event->getType());

  /// behavior
  event->setId(1);

  /// assert
  EXPECT_EQ(1, event->getId());
  EXPECT_EQ(1, event->getCreatorId());
  EXPECT_EQ(2, event->getGroupId());
  EXPECT_EQ(3, event->getGroupVersion());
  EXPECT_EQ("dummy tracking context", event->getTrackingContext());

  auto metaData = event->getMetaData();
  EXPECT_EQ(1, metaData.getId());
  EXPECT_EQ(1, metaData.getCreatorId());
  EXPECT_EQ(2, metaData.getGroupId());
  EXPECT_EQ(3, metaData.getGroupVersion());
  EXPECT_EQ("dummy tracking context", metaData.getTrackingContext());
}

TEST(EventTest, MetaDataGetWhatSet2) {
  /// init
  auto event = DummyEvent::createDummyEvent();

  /// behavior
  EventMetaData metaData;
  metaData.setId(1);
  metaData.setCreatorId(2);
  metaData.setGroupId(3);
  metaData.setGroupVersion(4);
  metaData.setTrackingContext("dummy tracking context");
  event->setPartialMetaData(metaData);

  /// assert
  EXPECT_EQ(1, event->getId());
  EXPECT_EQ(2, event->getCreatorId());
  EXPECT_EQ(3, event->getGroupId());
  EXPECT_EQ(4, event->getGroupVersion());
  EXPECT_EQ("dummy tracking context", event->getTrackingContext());
}

TEST(EventTest, MethodsDefaultImpl) {
  /// init
  auto event1 = DummyEvent::createDummyEvent();
  auto event2 = DummyEvent::createDummyEvent();

  /// assert
  EXPECT_TRUE((*event1).equals(*event2));
}

}  /// namespace gringofts::test

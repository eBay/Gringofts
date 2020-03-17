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

TEST(CommandTest, MetaDataGetWhatSet) {
  /// init
  auto command = DummyCommand::createDummyCommand();
  EXPECT_EQ(0, command->getType());

  /// behavior
  command->setId(1);

  /// assert
  EXPECT_EQ(1, command->getId());
  EXPECT_EQ(1, command->getCreatorId());
  EXPECT_EQ(2, command->getGroupId());
  EXPECT_EQ(3, command->getGroupVersion());
  EXPECT_EQ("dummy tracking context", command->getTrackingContext());
  EXPECT_EQ(nullptr, command->getRequestHandle());
}

TEST(CommandTest, MetaDataGetWhatSet2) {
  /// init
  auto command = DummyCommand::createDummyCommand();

  /// behavior
  CommandMetaData metaData;
  metaData.setId(1);
  metaData.setCreatorId(2);
  metaData.setGroupId(3);
  metaData.setGroupVersion(4);
  metaData.setTrackingContext("dummy tracking context");
  command->setPartialMetaData(metaData);

  /// assert
  EXPECT_EQ(1, command->getId());
  EXPECT_EQ(2, command->getCreatorId());
  EXPECT_EQ(3, command->getGroupId());
  EXPECT_EQ(4, command->getGroupVersion());
  EXPECT_EQ("dummy tracking context", command->getTrackingContext());
  EXPECT_EQ(nullptr, command->getRequestHandle());
}

TEST(CommandTest, MethodsDefaultImpl) {
  /// init
  auto command1 = DummyCommand::createDummyCommand();
  auto command2 = DummyCommand::createDummyCommand();

  /// assert
  EXPECT_TRUE((*command1).equals(*command2));
  EXPECT_EQ(Command::kVerifiedSuccess, command1->verifyCommand());
}

}  /// namespace gringofts::test

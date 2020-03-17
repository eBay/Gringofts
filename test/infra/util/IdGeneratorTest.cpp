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

#include "../../../src/infra/util/IdGenerator.h"

namespace gringofts::test {

class IdGeneratorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {}

  virtual void TearDown() {}

 protected:
  IdGenerator idGenerator;
};

TEST_F(IdGeneratorTest, DefaultIdIsZero) {
  // assert
  EXPECT_EQ(idGenerator.getCurrentId(), 0);
}

TEST_F(IdGeneratorTest, ThrowErrorIfSetSmallerId) {
  // init
  Id id1 = idGenerator.getCurrentId();

  // behavior
  Id id2 = idGenerator.getNextId();

  // assert
  EXPECT_GT(id2, id1);
  EXPECT_THROW(idGenerator.setCurrentId(id1), std::runtime_error);
}

TEST_F(IdGeneratorTest, NotThrowIfSetBiggerId) {
  // init
  Id id = idGenerator.getCurrentId();

  // assert
  EXPECT_NO_THROW(idGenerator.setCurrentId(id));
}

TEST_F(IdGeneratorTest, AlwaysMonotonicIncreaseId) {
  // init
  Id id1 = idGenerator.getCurrentId();
  Id id2 = idGenerator.getNextId();

  // behavior
  idGenerator.setCurrentId(id2);
  Id id3 = idGenerator.getNextId();

  // assert
  EXPECT_GT(id2, id1);
  EXPECT_GT(id3, id2);
}

}  /// namespace gringofts::test

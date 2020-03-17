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

#include "../../../../src/infra/es/store/SnapshotUtil.h"

namespace gringofts::test {

TEST(SnapshotUtilTest, GetLargestSnapshotTest) {
  /// init
  const auto &tempDir = testing::TempDir() + "/tests/";
  Util::executeCmd("mkdir -p " + tempDir + "tests2");
  Util::executeCmd("touch " + tempDir + "100.snapshot");
  Util::executeCmd("touch " + tempDir + "2.snapshot");
  Util::executeCmd("touch " + tempDir + "200.snapshot.tmp");
  Util::executeCmd("touch " + tempDir + "tests2/101.snapshot.tmp");
  Util::executeCmd("touch " + tempDir + "tests2/99.snapshot");

  /// behavior
  const auto &offset = SnapshotUtil::findLatestSnapshotOffset(tempDir);

  /// assert
  EXPECT_EQ(offset, 100);

  /// cleanup
  Util::executeCmd("rm -rf " + tempDir);
}

TEST(SnapshotUtilTest, GetLargestSnapshotTest2) {
  /// init
  const auto &tempDir = testing::TempDir() + "/tests/";
  Util::executeCmd("rm -rf " + tempDir);

  /// behavior
  const auto &offset = SnapshotUtil::findLatestSnapshotOffset(tempDir);

  /// assert
  EXPECT_FALSE(offset);
}

}  /// namespace gringofts::test

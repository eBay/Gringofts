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

#include "../../../src/infra/util/Util.h"

namespace gringofts::test {

TEST(UtilTest, ReleaseVersionTest) {
  /// init
  const char *testCWD =
      "/ebay/cronus/software/service_nodes"
      "/.ENV3rs710p46dq.fasrtprocessunit-app__ENV3rs710p46dq."
      "fasrtprocessunit-app__ENV3rs710p46dq-LVS-CLjsb6td10vi52g-10.149.253.56"
      "/installed-packages/magellan_trinidad_bas"
      "/1.0.1_2_1562809884239.unx/cronus";

  /// behavior
  const auto &version = Util::getReleaseVersion(testCWD);

  /// assert
  EXPECT_EQ(version, "1.0.1_2_1562809884239.unx");
}

TEST(UtilTest, UnknownVersionTest) {
  /// init
  const char *testCWD = "1.0.1_2_1562809884239.unx";

  /// behavior
  const auto &version = Util::getReleaseVersion(testCWD);

  /// assert
  EXPECT_EQ(version, "UNKNOWN");
}

}  /// namespace gringofts::test

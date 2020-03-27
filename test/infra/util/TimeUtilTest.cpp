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

#include "../../../src/infra/util/TimeUtil.h"

using gringofts::TimeUtil;

namespace gringofts::test {

TEST(TimeUtilTest, TwoCallsToTimeNanosWillNeverEqual) {
  /// init
  auto createdTimeInNanos1 = TimeUtil::currentTimeInNanos();
  /// deliberately sleep 1s so that OS in docker will not return the same value
  sleep(1);
  auto createdTimeInNanos2 = TimeUtil::currentTimeInNanos();

  /// assert
  EXPECT_LT(createdTimeInNanos1, createdTimeInNanos2);
}

TEST(TimeUtilTest, NanosToMillisTest) {
  /// init
  auto now = TimeUtil::currentTimeInNanos();

  /// assert
  EXPECT_EQ(TimeUtil::nanosToMillis(now), now / 1000000);
}

TEST(TimeUtilTest, ElapseTimeInMillisTest) {
  /// init
  auto now = TimeUtil::currentTimeInNanos();
  auto future = now + 1000000;

  /// assert
  EXPECT_EQ(TimeUtil::elapseTimeInMillis(now, future), 1);
  EXPECT_EQ(TimeUtil::elapseTimeInMillis(future, now), 0);
}

}  /// namespace gringofts::test

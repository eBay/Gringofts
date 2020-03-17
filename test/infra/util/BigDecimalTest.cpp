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

#include <iostream>

#include <gtest/gtest.h>

#include "../../../src/infra/util/BigDecimal.h"

namespace gringofts::test {

TEST(BigDecimalTest, OnePlusOneProblem) {
  auto result = BigDecimal(static_cast<uint64_t>(1)) + BigDecimal("1");
  EXPECT_EQ(result.toString(), "2.00");
  EXPECT_EQ(BigDecimal(2.0), result);
}

TEST(BigDecimalTest, BigInteger) {
  auto value1 = BigDecimal("1234567890123456789012345678901");
  auto value2 = BigDecimal("9876564564875456486545648546485487845");

  EXPECT_EQ(value1 + value2, BigDecimal("9876565799443346610002437558831166746"));
  EXPECT_EQ(value1 - value2, BigDecimal("-9876563330307566363088859534139808944"));
  EXPECT_EQ(value1 * value2, BigDecimal("12193289476526389375034113652217250284732602818075936909990708458345"));
}

TEST(BigDecimalTest, BigFloat) {
  auto value1 = BigDecimal("12345678.90123456789012345678901");
  auto value2 = BigDecimal("987656456487.5456486545648546485487845");

  EXPECT_EQ(value1 + value2, BigDecimal("987668802166.4468832224549781053377945"));
  EXPECT_EQ(value1 - value2, BigDecimal("-987644110808.6444140866747311917597745"));
  EXPECT_EQ((value1 * value2).toString(), "12193289476526389375.03");
}

}  /// namespace gringofts::test

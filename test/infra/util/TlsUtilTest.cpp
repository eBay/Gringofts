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

#include "../../../src/infra/util/TlsUtil.h"

namespace gringofts::test {

TEST(TlsUtilTest, BadConfigTest) {
  /// init
  INIReader reader("../test/infra/util/config/tls_bad.ini");

  /// assert
  EXPECT_THROW(TlsUtil::parseTlsConf(reader, "tls"), std::runtime_error);
}

TEST(TlsUtilTest, BuildServerCredentialsTest) {
  /// init
  INIReader reader("../test/infra/util/config/tls.ini");
  const auto &tlsConfOpt = TlsUtil::parseTlsConf(reader, "tls");

  /// assert
  EXPECT_TRUE(TlsUtil::buildServerCredentials(std::nullopt) != nullptr);
  EXPECT_TRUE(TlsUtil::buildServerCredentials(tlsConfOpt) != nullptr);
}

TEST(TlsUtilTest, BuildChannelCredentialsTest) {
  /// init
  INIReader reader("../test/infra/util/config/tls.ini");
  const auto &tlsConfOpt = TlsUtil::parseTlsConf(reader, "tls");

  /// assert
  EXPECT_TRUE(TlsUtil::buildChannelCredentials(std::nullopt) != nullptr);
  EXPECT_TRUE(TlsUtil::buildChannelCredentials(tlsConfOpt) != nullptr);
}

}  /// namespace gringofts::test

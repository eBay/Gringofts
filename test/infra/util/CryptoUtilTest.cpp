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
#include <openssl/conf.h>

#include "../../../src/infra/util/CryptoUtil.h"
#include "../../../src/infra/util/FileUtil.h"

namespace gringofts::test {

TEST(CryptoUtilTest, EncryptAndDecrypt) {
  /// A 256 bit key
  std::string key = "01234567890123456789012345678901";

  /// init CryptoUtil
  CryptoUtil cryptoTool;
  cryptoTool.init(key);

  /// Message to be encrypted
  std::string oldMessage = "The quick brown fox jumps over the lazy dog";

  std::string newMessage = oldMessage;

  /// from plain to cipher
  cryptoTool.encrypt(&newMessage);
  BIO_dump_fp(stdout, newMessage.c_str(), newMessage.size());

  /// from cipher to plain
  cryptoTool.decrypt(&newMessage);

  EXPECT_EQ(newMessage, oldMessage);
}

TEST(CryptoUtilTest, EncryptAndDecrypt2) {
  /// init
  std::string key = "01234567890123456789012345678901";

  CryptoUtil cryptoTool;
  cryptoTool.init(key);

  /// behavior
  std::ofstream ofs{"file", std::ios::trunc | std::ios::binary};
  cryptoTool.beginEncryption(1);
  cryptoTool.encryptUint64ToFile(ofs, 12);
  cryptoTool.encryptStrToFile(ofs, "part 1");
  cryptoTool.encryptUint64ToFile(ofs, 2);
  cryptoTool.commitEncryption(ofs);
  ofs.close();

  std::ifstream ifs{"file", std::ios::binary};
  cryptoTool.beginDecryption(1);
  auto intVal = cryptoTool.decryptUint64FromFile(ifs);
  auto strVal = cryptoTool.decryptStrFromFile(ifs);
  auto intVal2 = cryptoTool.decryptUint64FromFile(ifs);
  cryptoTool.commitDecryption(ifs);
  ifs.close();

  FileUtil::deleteFile("file");

  /// assert
  EXPECT_EQ(intVal, 12);
  EXPECT_EQ(strVal, "part 1");
  EXPECT_EQ(intVal2, 2);
}

/// TODO: Re-enable this test
TEST(CryptoUtilTest, DISABLED_ExitScenariosTest) {
  /// init
  const auto &reader = INIReader("../test/infra/util/config/aes.ini");
  CryptoUtil cryptoTool;
  cryptoTool.init(reader);

  /// assert
  EXPECT_EXIT(cryptoTool.init("01234567890123456789012345678901"), ::testing::ExitedWithCode(1), "");
  EXPECT_EXIT(cryptoTool.init(reader), ::testing::ExitedWithCode(1), "");
}

TEST(CryptoUtilTest, HMAC) {
  /**
   * `echo -n "The quick brown fox jumps over the lazy dog" | openssl dgst -sha256 -mac hmac -macopt key:01234567890123456789012345678901`
   * the output digest in hex format is "3b3803c66fe6688cc3ddcc02ca7fa6fd940a326cefbd1e3606c42eccbb21bdba"
   */
  std::string key = "01234567890123456789012345678901";
  std::string message = "The quick brown fox jumps over the lazy dog";
  std::string expectDigest = "3b3803c66fe6688cc3ddcc02ca7fa6fd940a326cefbd1e3606c42eccbb21bdba";

  /// init
  CryptoUtil crypto;
  crypto.init(key);

  auto digest1 = crypto.hmac(message);
  auto digest2 = crypto.hmac(reinterpret_cast<const unsigned char *>(message.c_str()),
                             message.size());

  SPDLOG_INFO("digest in hex format: {}", StrUtil::hexStr(digest1));

  EXPECT_EQ(digest1, digest2);
  EXPECT_EQ(StrUtil::hexStr(digest1), expectDigest);
}

}  /// namespace gringofts::test

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

class CryptoUtilTest : public ::testing::Test {
 protected:
  static const SecKeyVersion defaultVersion = 1;
};

TEST_F(CryptoUtilTest, EncryptAndDecrypt) {
  /// A 256 bit key
  std::string key = "01234567890123456789012345678901";

  /// init CryptoUtil
  CryptoUtil cryptoTool;
  cryptoTool.init(defaultVersion, key);

  /// Message to be encrypted
  std::string oldMessage = "The quick brown fox jumps over the lazy dog";

  std::string newMessage = oldMessage;

  /// from plain to cipher
  cryptoTool.encrypt(&newMessage, cryptoTool.getLatestSecKeyVersion());
  BIO_dump_fp(stdout, newMessage.c_str(), newMessage.size());

  /// from cipher to plain
  cryptoTool.decrypt(&newMessage, cryptoTool.getLatestSecKeyVersion());

  EXPECT_EQ(newMessage, oldMessage);
}

TEST_F(CryptoUtilTest, DISABLED_ExitScenariosTest) {
  /// init
  const auto &reader = INIReader("../test/infra/util/config/aes.ini");
  CryptoUtil cryptoTool;
  cryptoTool.init(reader);

  /// assert
  EXPECT_EXIT(cryptoTool.init(defaultVersion, "01234567890123456789012345678901"), ::testing::ExitedWithCode(1), "");
  EXPECT_EXIT(cryptoTool.init(reader), ::testing::ExitedWithCode(1), "");
}

TEST_F(CryptoUtilTest, HMAC) {
  /**
   * `echo -n "The quick brown fox jumps over the lazy dog" | openssl dgst -sha256 -mac hmac -macopt key:01234567890123456789012345678901`
   * the output digest in hex format is "3b3803c66fe6688cc3ddcc02ca7fa6fd940a326cefbd1e3606c42eccbb21bdba"
   */
  std::string key = "01234567890123456789012345678901";
  std::string message = "The quick brown fox jumps over the lazy dog";
  std::string expectDigest = "3b3803c66fe6688cc3ddcc02ca7fa6fd940a326cefbd1e3606c42eccbb21bdba";

  /// init
  CryptoUtil crypto;
  crypto.init(defaultVersion, key);

  auto digest1 = crypto.hmac(message, crypto.getLatestSecKeyVersion());
  auto digest2 = crypto.hmac(reinterpret_cast<const unsigned char *>(message.c_str()),
      message.size(), crypto.getLatestSecKeyVersion());

  SPDLOG_INFO("digest in hex format: {}", StrUtil::hexStr(digest1));

  EXPECT_EQ(digest1, digest2);
  EXPECT_EQ(StrUtil::hexStr(digest1), expectDigest);
}

TEST_F(CryptoUtilTest, EncryptDecryptWithMultiVersion) {
  /// init
  const auto &reader = INIReader("../test/infra/util/config/aes.ini");
  CryptoUtil cryptoTool;
  cryptoTool.init(reader);

  auto versions = cryptoTool.getDescendingVersions();
  EXPECT_EQ(versions.size(), 3);

  /// test all versions
  for (auto v : versions) {
    for (auto anotherV : versions) {
      if (anotherV == v) {
        continue;
      }
      std::string originalMsg = "this is to test secret keys using version " + std::to_string(v);
      std::string msgToEncrypt = originalMsg;
      EXPECT_EQ(cryptoTool.encrypt(&msgToEncrypt, v), 0);

      std::string decryptedMsgWithRightVersion = msgToEncrypt;
      SPDLOG_INFO("decrypting using version: {}", v);
      EXPECT_EQ(cryptoTool.decrypt(&decryptedMsgWithRightVersion, v), 0);
      std::string decryptedMsgWithWrongVersion = msgToEncrypt;
      SPDLOG_INFO("decrypting using wrong version: {}", anotherV);
      EXPECT_EQ(cryptoTool.decrypt(&decryptedMsgWithWrongVersion, anotherV), -1);

      EXPECT_EQ(decryptedMsgWithRightVersion, originalMsg);
      EXPECT_NE(decryptedMsgWithWrongVersion, originalMsg);

      SPDLOG_INFO("hmac using version: {}", v);
      auto digestWithRightVersion = cryptoTool.hmac(originalMsg, v);
      SPDLOG_INFO("hmac using version: {}", anotherV);
      auto digestWithWrongVersion = cryptoTool.hmac(originalMsg, anotherV);

      EXPECT_NE(digestWithRightVersion, digestWithWrongVersion);
    }
  }
}

}  /// namespace gringofts::test

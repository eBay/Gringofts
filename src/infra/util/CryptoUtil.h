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

#ifndef SRC_INFRA_UTIL_CRYPTOUTIL_H_
#define SRC_INFRA_UTIL_CRYPTOUTIL_H_

#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/hmac.h>

#include <INIReader.h>
#include <spdlog/spdlog.h>

#include "../es/Crypto.h"

namespace gringofts {

/**
 * This class is a wrapper for openssl functions, which is used to
 * 1) encrypt and decrypt raft payload, via AES CBC 256
 * 2) generate hmac, via SHA256. BTW, SHA256 reuses key of AES CBC 256,
 *    since it can accept key with any len.
 */
class CryptoUtil : public Crypto {
 public:
  CryptoUtil() {
    /// set up IV
    memset(mIV, 0x00, AES_BLOCK_SIZE);
  }

  CryptoUtil(const CryptoUtil &) = delete;
  CryptoUtil &operator=(const CryptoUtil &) = delete;

  void init(const INIReader &reader);
  void init(const std::string &key);

  /// do a in-place encryption on std::string,
  /// if not enabled, do nothing.
  void encrypt(std::string *) const;

  /// do a in-place decryption on std::string,
  /// if not enabled, do nothing.
  void decrypt(std::string *) const;

  /// return hmac_sha256, if not enabled, return empty str.
  std::string hmac(const std::string &) const;

  /// return hmac_sha256 for n bytes at d, if not enabled, return empty str.
  std::string hmac(const unsigned char *d, std::size_t n) const;

  void beginEncryption(uint64_t bufferSize) override;
  void encryptUint64ToFile(std::ofstream &ofs, uint64_t content) override;
  void encryptStrToFile(std::ofstream &ofs, const std::string &content) override;
  void commitEncryption(std::ofstream &ofs) override;

  void beginDecryption(uint64_t bufferSize) override;
  uint64_t decryptUint64FromFile(std::ifstream &ifs) override;
  std::string decryptStrFromFile(std::ifstream &ifs) override;
  void commitDecryption(std::ifstream &ifs) override;

 private:
  /// decode aes key from base64 to raw bytes
  static void decodeBase64Key(const std::string &base64,
                              unsigned char *key, int keyLen);

  /// print error msg from openssl and abort
  static void handleErrors();

  /// plain text -> cipher text
  /// return len of cipher text
  static int encrypt(const unsigned char *plain, int plainLen,
                     const unsigned char *key, const unsigned char *iv,
                     unsigned char *cipher);

  /// cipher text -> plain text
  /// return len of plain text
  static int decrypt(const unsigned char *cipher, int cipherLen,
                     const unsigned char *key, const unsigned char *iv,
                     unsigned char *plain);

  /// Whether aes feature is enable
  bool mEnable = false;

  /// Use EVP_aes_256_cbc, 32 bytes equals 256 bit
  static constexpr uint64_t kKeyLen = 32;
  /// A 256 bit key
  unsigned char mKey[kKeyLen];

  /// A 128 bit IV
  unsigned char mIV[AES_BLOCK_SIZE];

  static constexpr auto kPlainMaxLen = 1 << 30;  // 1G

  std::vector<unsigned char> mPlainBuffer;
  uint64_t mPlainBufferSize = 0;

  uint64_t mCurrentOffset = 0;

  std::vector<unsigned char> mCipherBuffer;
  uint64_t mCipherBufferSize = 0;

  /// control disk I/O rate of taking snapshot by
  /// limiting the frequency of flushing cipher buffer
  uint64_t mLastFlushTimeInNano = 0;

  EVP_CIPHER_CTX *mCtx;

  void bufferOrEncrypt(std::ofstream &ofs, const unsigned char *content, size_t length);
  void readOrDecrypt(std::ifstream &ifs, unsigned char *content, size_t length);
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_CRYPTOUTIL_H_

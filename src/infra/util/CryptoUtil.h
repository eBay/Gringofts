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

#include <algorithm>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/hmac.h>

#include <INIReader.h>
#include <spdlog/spdlog.h>

#include "../monitor/MonitorTypes.h"

namespace gringofts {

using SecKeyVersion = uint64_t;

struct SecretKey {
  /// Use EVP_aes_256_cbc, 32 bytes equals 256 bit
  static constexpr uint64_t kKeyLen = 32;
  static constexpr SecKeyVersion kInvalidSecKeyVersion = 0;

  SecKeyVersion mVersion = kInvalidSecKeyVersion;
  /// A 256 bit key
  unsigned char mKey[kKeyLen] = {0};
};

/**
 * This class is a wrapper for openssl functions, which is used to
 * 1) encrypt and decrypt raft payload, via AES CBC 256
 * 2) generate hmac, via SHA256. BTW, SHA256 reuses key of AES CBC 256,
 *    since it can accept key with any len.
 */
class CryptoUtil {
 public:
  CryptoUtil() : mLatestVersionGauge(getGauge("key_version", {})) {
    /// set up IV
    memset(mIV, 0x00, AES_BLOCK_SIZE);
  }

  CryptoUtil(const CryptoUtil &) = delete;
  CryptoUtil &operator=(const CryptoUtil &) = delete;

  void init(const INIReader &reader);
  void init(SecKeyVersion version, const std::string &key);

  bool isEnabled() { return mEnabled; }
  /// sorted versions in ascending order
  const std::vector<SecKeyVersion>& getDescendingVersions() const {
    return mDescendingSecKeyVersions;
  }
  SecKeyVersion getLatestSecKeyVersion() const { return mLatestVersion; }

  /// do a in-place encryption on std::string,
  /// if not enabled, do nothing.
  /// return 0 if success
  int encrypt(std::string *, SecKeyVersion version) const;

  /// do a in-place decryption on std::string using the specified key version,
  /// if not enabled, do nothing.
  /// return 0 if success
  int decrypt(std::string *payload, SecKeyVersion version) const;

  /// return hmac_sha256 using the specified key version, if not enabled, return empty str.
  std::string hmac(const std::string &, SecKeyVersion version) const;

  /// return hmac_sha256 for n bytes at d using the specified key version, if not enabled, return empty str.
  std::string hmac(const unsigned char *d, std::size_t n, SecKeyVersion version) const;

 private:
  void assertValidVersion(SecKeyVersion version) const;
  /// decode aes key from base64 to raw bytes
  static void decodeBase64Key(const std::string &base64,
                              unsigned char *key, int keyLen);

  /// print error msg from openssl and abort
  static int handleErrors();

  /// plain text -> cipher text
  /// cipher: encrypted text
  /// cipherLen: encrypted text length
  /// return 0 if success
  static int encrypt(const unsigned char *plain, int plainLen,
                     const unsigned char *key, const unsigned char *iv,
                     unsigned char *cipher, int *cipherLen);

  /// cipher text -> plain text
  /// plain: decrypted text
  /// plainLen: decrypted text length
  /// return 0 if success
  static int decrypt(const unsigned char *cipher, int cipherLen,
                     const unsigned char *key, const unsigned char *iv,
                     unsigned char *plain, int *plainLen);

  /// Whether aes feature is enable
  bool mEnabled = false;

  std::vector<SecKeyVersion> mDescendingSecKeyVersions;
  std::map<SecKeyVersion, SecretKey> mAllKeys;
  SecKeyVersion mLatestVersion = SecretKey::kInvalidSecKeyVersion;
  /// A 128 bit IV
  unsigned char mIV[AES_BLOCK_SIZE];

  mutable santiago::MetricsCenter::GaugeType mLatestVersionGauge;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_CRYPTOUTIL_H_

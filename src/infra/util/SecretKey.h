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

#ifndef SRC_INFRA_UTIL_SECRETKEY_H_
#define SRC_INFRA_UTIL_SECRETKEY_H_

#include <algorithm>
#include <atomic>
#include <map>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include "../monitor/MonitorTypes.h"

namespace gringofts {

using SecKeyVersion = uint64_t;

/**
 * This class is used to fetch keys from a given source
 * Version of keys is monotonic increasing
 * This class is thread safe
 */
class SecretKey {
 public:
  SecretKey() : mLatestVersionGauge(getGauge("key_version", {})) {}
  SecretKey(const SecretKey &) = delete;
  SecretKey &operator=(const SecretKey &) = delete;
  virtual ~SecretKey() = default;

  virtual SecKeyVersion getLatestSecKeyVersion() { return mLatestVersion.load(); }
  virtual const unsigned char * getKeyByVersion(SecKeyVersion version) {
    assert(version <= mLatestVersion);
    return mAllKeys[version].data();
  }

  /// Use EVP_aes_256_cbc, 32 bytes equals 256 bit
  static constexpr uint64_t kKeyLen = 32;
  static constexpr SecKeyVersion kInvalidSecKeyVersion = 0;
  static constexpr SecKeyVersion kOldestSecKeyVersion = 1;
  using KeyBytes = std::array<unsigned char, kKeyLen>;

 protected:
  /// decode aes key from base64 to raw bytes
  void decodeBase64Key(const std::string &base64, unsigned char *key, int keyLen) {
    BIO *bio;
    BIO *b64;

    bio = BIO_new_mem_buf(base64.c_str(), base64.length());
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);  // Do not use newlines to flush buffer

    auto len = BIO_read(bio, key, keyLen);
    if (len != keyLen) {
      // len should equal keyLen, else something went horribly wrong
      SPDLOG_ERROR("Decode AES key error. expected: {}, actual: {}", kKeyLen, len);
      exit(1);
    }
    BIO_free_all(bio);
  }

  std::vector<KeyBytes> mAllKeys;
  std::atomic<SecKeyVersion> mLatestVersion = kInvalidSecKeyVersion;
  mutable santiago::MetricsCenter::GaugeType mLatestVersionGauge;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_SECRETKEY_H_

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

#ifndef SRC_INFRA_UTIL_FILESECRETKEY_H_
#define SRC_INFRA_UTIL_FILESECRETKEY_H_

#include "SecretKey.h"

namespace gringofts {

/**
 * This class load key from a given file.
 * File format looks like:
 * 
 * keys { version: 1 key: "base64 encoded key content" }
 * keys { version: 2 key: "base64 encoded key content" }
 * keys { version: 3 key: "base64 encoded key content" }
 * ...
 * 
 */
class FileSecretKey : public SecretKey {
 public:
  FileSecretKey() = delete;
  explicit FileSecretKey(const std::string &keyFilePath);
  ~FileSecretKey() override = default;

  const unsigned char * getKeyByVersion(SecKeyVersion version) override;

 private:
  void assertValidVersion(SecKeyVersion version) const;

  const std::string mKeyFilePath;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_FILESECRETKEY_H_

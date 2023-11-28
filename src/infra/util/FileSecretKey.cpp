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

#include "FileSecretKey.h"

#include <google/protobuf/text_format.h>

#include "FileUtil.h"
#include "../es/store/generated/store.pb.h"

namespace gringofts {

FileSecretKey::FileSecretKey(const std::string &keyFilePath)
  : mKeyFilePath(keyFilePath) {
  std::string content = FileUtil::getFileContent(mKeyFilePath);
  es::EncryptSecKeySet keySet;
  assert(google::protobuf::TextFormat::ParseFromString(content, &keySet));

  mAllKeys.clear();
  mLatestVersion = kInvalidSecKeyVersion;
  SecKeyVersion oldestVersion = kInvalidSecKeyVersion;

  std::set<google::protobuf::uint64> allVersions;
  for (auto keyAndVersion : keySet.keys()) {
    allVersions.emplace(keyAndVersion.version());
  }

  mAllKeys.resize(keySet.keys_size() + 1);

  for (auto keyAndVersion : keySet.keys()) {
    auto version = keyAndVersion.version();
    assert(version > kInvalidSecKeyVersion);

    decodeBase64Key(keyAndVersion.key(), mAllKeys[version].data(), kKeyLen);
    if (version > mLatestVersion) {
      mLatestVersion = version;
    }
    if (oldestVersion == kInvalidSecKeyVersion || version < oldestVersion) {
      oldestVersion = version;
    }
  }

  assert(mLatestVersion != kInvalidSecKeyVersion);
  assert(oldestVersion == kOldestSecKeyVersion);
  assert(mLatestVersion == allVersions.size());
  assert(!mAllKeys.empty());
  mLatestVersionGauge.set(mLatestVersion);

  SPDLOG_INFO("{} AES keys loaded from file {}, the latest version is {}",
      mAllKeys.size() - 1, mKeyFilePath, mLatestVersion);
}

const unsigned char * FileSecretKey::getKeyByVersion(SecKeyVersion version) {
  assertValidVersion(version);
  return mAllKeys[version].data();
}

void FileSecretKey::assertValidVersion(SecKeyVersion version) const {
  if (version > mLatestVersion) {
    SPDLOG_ERROR("Invalid key version: {}", version);
    exit(1);
  }
}

}  /// namespace gringofts

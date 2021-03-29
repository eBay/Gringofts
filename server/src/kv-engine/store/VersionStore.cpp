/************************************************************************
Copyright 2021-2022 eBay Inc.
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

#include <spdlog/spdlog.h>

#include "VersionStore.h"

namespace goblin::kvengine::store {

VersionStore::VersionStore() {
}

VersionType VersionStore::allocateNextVersion(OnNewVersionCallBack cb, VersionType expectedVersion) {
  std::lock_guard<std::mutex> lock(mNextVersionMutex);
  VersionType newVersion = kInvalidVersion;
  if (expectedVersion == kInvalidVersion) {
    newVersion = mNextVersion++;
  } else if (mNextVersion >= expectedVersion) {
    newVersion = expectedVersion;
  } else {
    newVersion = expectedVersion;
    mNextVersion = expectedVersion;
  }
  cb(newVersion);
  return newVersion;
}

VersionType VersionStore::getMaxAllocatedVersion() {
  return mNextVersion - 1;
}

VersionType VersionStore::getCurMaxVersion() {
  return mMaxVersion;
}

void VersionStore::updateCurMaxVersion(const VersionType &newVersion) {
  if (newVersion < mMaxVersion) {
    return;
  }
  std::lock_guard<std::mutex> lock(mMaxVersionMutex);
  mMaxVersion = std::max(newVersion, mMaxVersion.load());
}

void VersionStore::recoverMaxVersion(const VersionType &version) {
  SPDLOG_INFO("recover max version from {} to {}", mMaxVersion.load(), version);
  mMaxVersion = version;
  mNextVersion = mMaxVersion + 1;
}

}  /// namespace goblin::kvengine::store


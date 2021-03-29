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

#ifndef SERVER_SRC_KV_ENGINE_STORE_VERSIONSTORE_H_
#define SERVER_SRC_KV_ENGINE_STORE_VERSIONSTORE_H_

#include <atomic>
#include <functional>
#include <string>

#include "../utils/Status.h"
#include "../store/KVStore.h"

namespace goblin::kvengine::store {

class VersionStore {
 public:
  typedef std::function<void(const store::VersionType &version)> OnNewVersionCallBack;

  VersionStore();
  VersionStore(const VersionStore &) = delete;
  VersionStore &operator=(const VersionStore &) = delete;

  virtual ~VersionStore() = default;

  /// automically allocate a new version and invoke the callback
  /// the cb should be short
  VersionType allocateNextVersion(OnNewVersionCallBack cb, VersionType expectedVersion = kInvalidVersion);
  VersionType getMaxAllocatedVersion();
  VersionType getCurMaxVersion();
  void updateCurMaxVersion(const VersionType &newVersion);

  void recoverMaxVersion(const VersionType &maxVersion);

  static constexpr VersionType kInvalidVersion = 0;

 private:
  std::mutex mNextVersionMutex;
  std::mutex mMaxVersionMutex;
  /// used to allocate a new version
  std::atomic<VersionType> mNextVersion = 0;
  /// indicate the committed version
  std::atomic<VersionType> mMaxVersion = 0;
};

}  /// namespace goblin::kvengine::store

#endif  // SERVER_SRC_KV_ENGINE_STORE_VERSIONSTORE_H_


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

#ifndef SRC_INFRA_UTIL_IDGENERATOR_H_
#define SRC_INFRA_UTIL_IDGENERATOR_H_

#include <atomic>

#include "../common_types.h"

namespace gringofts {

/**
 * An util class to generate monotonically increasing id.
 * Strict incremental id is not guaranteed.
 * Thread safety is also not guaranteed. Please confine the usage to one thread or use lock.
 */
class IdGenerator final {
 public:
  IdGenerator();
  ~IdGenerator() = default;

  /**
   * Get the next available id
   * @return next available id
   */
  Id getNextId() {
    mCurrentId += 1;
    return mCurrentId;
  }

  /**
   * Get the current highest id which is used
   * @return current highest id which is used
   */
  Id getCurrentId() const {
    return mCurrentId;
  }

  /**
   * Set the current highest id which is used. This function has two use scenarios:
   * 1. During class initialization
   * 2. During application recovery, when the last used id will be loaded from command/event store
   * @param id the new highest id which is used
   * @throw runtime_error if \p id is smaller than current highest id
   */
  void setCurrentId(const Id id);

 private:
  /**
   * The current highest id which has been used
   */
  std::atomic<Id> mCurrentId;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_IDGENERATOR_H_

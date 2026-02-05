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

#pragma once

#include <spdlog/spdlog.h>

namespace gringofts {
namespace raft {

template <typename T>
class DoubleBuffer {
  // only workable for single thread
 public:
  DoubleBuffer() {
    SPDLOG_INFO("Construct DoubleBuffer");
    mBuffer[0] = std::make_shared<T>();
    mBuffer[1] = std::make_shared<T>();
  }
  ~DoubleBuffer() = default;

  std::shared_ptr<const T> constCurrent() const {
    return mBuffer[mCurrentLoc];
  }

  std::shared_ptr<T> getCurrent() {
    return mBuffer[mCurrentLoc];
  }

  uint32_t getCurrentLoc() const {
    return mCurrentLoc;
  }

  std::shared_ptr<T> getBypass() {
    return mBuffer[1 - mCurrentLoc];
  }

  uint32_t getBypassLoc() const {
    return 1 - mCurrentLoc;
  }

  void swap() {
    mCurrentLoc = 1 - mCurrentLoc;
    SPDLOG_INFO("Swap DoubleBuffer, new location: {}", mCurrentLoc);
  }

 private:
  std::shared_ptr<T> mBuffer[2];
  uint32_t mCurrentLoc = 0;
};

}  // namespace raft
}  // namespace gringofts

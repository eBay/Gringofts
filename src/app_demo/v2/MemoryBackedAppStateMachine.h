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

#ifndef SRC_APP_DEMO_V2_MEMORYBACKEDAPPSTATEMACHINE_H_
#define SRC_APP_DEMO_V2_MEMORYBACKEDAPPSTATEMACHINE_H_

#include <optional>

#include "AppStateMachine.h"
#include "RocksDBBackedAppStateMachine.h"

namespace gringofts {
namespace demo {
namespace v2 {

class MemoryBackedAppStateMachine : public v2::AppStateMachine {
 public:
  /**
   * implement getter() and setter()
   */
  void setValue(uint64_t value) override {
    assert(mValue + 1 == value);
    SPDLOG_INFO("set value from {} to {}", mValue, value);

    mValue = value;
  }

  uint64_t getValue() const override { return mValue; }

  /**
   * integration
   */
  void swapState(StateMachine *anotherStateMachine) override {
    auto &another = dynamic_cast<RocksDBBackedAppStateMachine &>(*anotherStateMachine);
    mValue = another.mValue;

    /// explicitly trigger a flush in RocksDB
    another.flushToRocksDB();
  }
};

}  /// namespace v2
}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_V2_MEMORYBACKEDAPPSTATEMACHINE_H_

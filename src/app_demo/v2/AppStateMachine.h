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

#ifndef SRC_APP_DEMO_V2_APPSTATEMACHINE_H_
#define SRC_APP_DEMO_V2_APPSTATEMACHINE_H_

#include "../AppStateMachine.h"

namespace gringofts {
namespace demo {
namespace v2 {

class AppStateMachine : public demo::AppStateMachine {
 public:
  struct RocksDBConf {
    /// RocksDB key
    static constexpr const char *kLastAppliedIndexKey = "last_applied_index";
    static constexpr const char *kValueKey = "value";
  };

  /**
   * integration
   */
  void clearState() override { mValue = 0; }

  /// unit test
  bool hasSameState(const StateMachine &) const override { return true; }

 protected:
  /// state owned by both Memory-backed SM and RocksDB-backed SM
  uint64_t mValue = 0;
};

}  /// namespace v2
}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_V2_APPSTATEMACHINE_H_

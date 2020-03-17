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
  /**
   * v2::AppStateMachine does not support snapshot
   */
  bool createSnapshotAndPersist(uint64_t offset,
                                std::ofstream &ofs,
                                Crypto &crypto) const override { assert(0); }

  std::optional<uint64_t> loadSnapshotFrom(std::ifstream &ifs,
                                           const CommandDecoder &commandDecoder,
                                           const EventDecoder &eventDecoder,
                                           Crypto &crypto) override { assert(0); }
  /**
   * integration
   */
  void clearState() override { mValue = 0; }
  virtual uint64_t recoverSelf() = 0;

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

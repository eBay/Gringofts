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

#ifndef SRC_APP_DEMO_V1_APPSTATEMACHINE_H_
#define SRC_APP_DEMO_V1_APPSTATEMACHINE_H_

#include "../AppStateMachine.h"

namespace gringofts {
namespace demo {
namespace v1 {

class AppStateMachine final : public demo::AppStateMachine {
 public:
  AppStateMachine() = default;
  ~AppStateMachine() override = default;

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
   * implement snapshot
   */
  bool createSnapshotAndPersist(uint64_t offset,
                                std::ofstream &ofs,
                                Crypto &crypto) const override;

  std::optional<uint64_t> loadSnapshotFrom(std::ifstream &ifs,
                                      const CommandDecoder &commandDecoder,
                                      const EventDecoder &eventDecoder,
                                      Crypto &crypto) override;

  /**
   * integration
   */
  void swapState(StateMachine *anotherStateMachine) override {
    auto &another = dynamic_cast<v1::AppStateMachine &>(*anotherStateMachine);
    mValue = another.mValue;
  }

  void clearState() override { mValue = 0; }

  bool hasSameState(const StateMachine &anotherStateMachine) const override {
    auto &another = dynamic_cast<const v1::AppStateMachine &>(anotherStateMachine);
    return another.mValue == mValue;
  }

 private:
  uint64_t mValue = 0;
};

}  /// namespace v1
}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_V1_APPSTATEMACHINE_H_

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

#ifndef SRC_APP_DEMO_APPSTATEMACHINE_H_
#define SRC_APP_DEMO_APPSTATEMACHINE_H_

#include "execution/IncreaseApplier.h"
#include "execution/IncreaseHandler.h"
#include "should_be_generated/domain/IncreaseCommand.h"
#include "should_be_generated/domain/ProcessedEvent.h"
#include "../app_util/AppStateMachine.h"

namespace gringofts {
namespace demo {

class AppStateMachine : public gringofts::app::AppStateMachine {
 public:
  AppStateMachine();
  ~AppStateMachine() override = default;

  ProcessHint processCommandAndApply(const Command &command, std::vector<std::shared_ptr<Event>> *events) override;

  /**
   * define getter() and setter()
   */
  virtual void setValue(uint64_t value) = 0;
  virtual uint64_t getValue() const = 0;

  /// due to EventApplyLoop<StateMachineType>::replayTillNoEvent()
  void generateMockData() {}

 private:
  /**
   * implement process() and apply() which will utilize
   * getter() and setter() to manipulate state.
   */
  ProcessHint process(const IncreaseCommand &command,
                      std::vector<std::shared_ptr<Event>> *events) const {
    IncreaseHandler handler;
    return handler.process(*this, command, events);
  }

  StateMachine &apply(const ProcessedEvent &event) {
    IncreaseApplier applier;
    applier.apply(event, this);
    return *this;
  }
};

}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_APPSTATEMACHINE_H_

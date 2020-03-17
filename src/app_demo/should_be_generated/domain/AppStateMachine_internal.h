// THIS FILE SHOULD BE AUTO-GENERATED, PLEASE DO NOT EDIT!!!
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

#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_APPSTATEMACHINE_INTERNAL_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_APPSTATEMACHINE_INTERNAL_H_

#include <typeindex>

#include "../../../infra/es/ProcessCommandStateMachine.h"

#include "IncreaseCommand.h"
#include "ProcessedEvent.h"

namespace gringofts {
namespace demo {

class AppStateMachine_internal : public ProcessCommandStateMachine {
 public:
  AppStateMachine_internal();

  ~AppStateMachine_internal() = default;

  // disallow copy ctor and copy assignment
  AppStateMachine_internal(const AppStateMachine_internal &) = delete;
  AppStateMachine_internal &operator=(const AppStateMachine_internal &) = delete;

  // disallow move ctor and move assignment
  AppStateMachine_internal(AppStateMachine_internal &&) = delete;
  AppStateMachine_internal &operator=(AppStateMachine_internal &&) = delete;

  ProcessHint processCommand(const Command &, std::vector<std::shared_ptr<Event>> *) const override;
  ProcessHint processCommandAndApply(const Command &, std::vector<std::shared_ptr<Event>> *) override;

  StateMachine &applyEvent(const Event &) override;

 private:
  using CommandProcessor = ProcessHint(AppStateMachine_internal::*)(const Command &,
                                                                    const StateMachine &,
                                                                    std::vector<std::shared_ptr<Event>> *) const;
  std::unordered_map<const std::type_info *, CommandProcessor> mCommandProcessorMap;

  using EventApplier = StateMachine &(AppStateMachine_internal::*)(const Event &);  // NOLINT [runtime/casting]
  std::unordered_map<const std::type_info *, EventApplier> mEventApplierMap;

  void registerCommandProcessor(const std::type_info *commandType, CommandProcessor commandProcessor) {
    mCommandProcessorMap.emplace(commandType, commandProcessor);
  }
  void registerEventApplier(const std::type_info *eventType, EventApplier eventApplier) {
    mEventApplierMap.emplace(eventType, eventApplier);
  }

 private:
  ProcessHint processIncreaseCommand(const Command &,
                                     const StateMachine &,
                                     std::vector<std::shared_ptr<Event>> *) const;
  StateMachine &applyProcessedEvent(const Event &);

 protected:
  virtual ProcessHint process(const IncreaseCommand &, std::vector<std::shared_ptr<Event>> *) const = 0;
  virtual StateMachine &apply(const ProcessedEvent &) = 0;
};

}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_APPSTATEMACHINE_INTERNAL_H_

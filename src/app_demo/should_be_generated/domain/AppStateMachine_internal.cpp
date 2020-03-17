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

#include "AppStateMachine_internal.h"

#include <spdlog/spdlog.h>

namespace gringofts {
namespace demo {

AppStateMachine_internal::AppStateMachine_internal() {
  registerCommandProcessor(&typeid(IncreaseCommand), &AppStateMachine_internal::processIncreaseCommand);
  registerEventApplier(&typeid(ProcessedEvent), &AppStateMachine_internal::applyProcessedEvent);
}

ProcessHint AppStateMachine_internal::processCommand(const Command &command,
                                                     std::vector<std::shared_ptr<Event>> *events) const {
  const std::type_info *commandType = &typeid(command);
  const auto entry = mCommandProcessorMap.find(commandType);
  if (entry != mCommandProcessorMap.end()) {
    CommandProcessor commandProcessor = entry->second;
    return (this->*commandProcessor)(command, *this, events);
  }

  SPDLOG_WARN("Cannot find command processor for {}", commandType->name());
  return ProcessHint{503, "Cannot find command processor for " + std::string(commandType->name())};
}

ProcessHint AppStateMachine_internal::processCommandAndApply(const Command &command,
                                                             std::vector<std::shared_ptr<Event>> *events) {
  auto hint = processCommand(command, events);
  if (events->empty()) {
    return hint;
  }

  for (const auto event : *events) {
    applyEvent(*event);
  }
  return hint;
}

ProcessHint AppStateMachine_internal::processIncreaseCommand(const Command &command,
                                                             const StateMachine &stateMachine,
                                                             std::vector<std::shared_ptr<Event>> *events) const {
  auto &c = dynamic_cast<const IncreaseCommand &>(command);
  return this->process(c, events);
}

StateMachine &AppStateMachine_internal::applyProcessedEvent(const Event &event) {
  auto &c = dynamic_cast<const ProcessedEvent &>(event);
  return this->apply(c);
}

StateMachine &AppStateMachine_internal::applyEvent(const Event &event) {
  SPDLOG_DEBUG("applying event {}", typeid(event).name());
  const std::type_info *eventType = &typeid(event);
  const auto entry = mEventApplierMap.find(eventType);
  if (entry != mEventApplierMap.end()) {
    EventApplier eventApplier = entry->second;
    return (this->*eventApplier)(event);
  }

  SPDLOG_WARN("Cannot find event applier for {}", eventType->name());
  return *this;
}

}  /// namespace demo
}  /// namespace gringofts

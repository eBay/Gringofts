/************************************************************************
Copyright 2019-2022 eBay Inc.
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

#include "AppStateMachine.h"
#include "should_be_generated/domain/common_types.h"

namespace gringofts::demo {

AppStateMachine::AppStateMachine() {
  registerCommandProcessor(INCREASE_COMMAND, [this](const gringofts::Command &command,
                                                    std::vector<std::shared_ptr<gringofts::Event>> *events) {
    return this->process(dynamic_cast<const IncreaseCommand&>(command), events);
  });

  registerEventApplier(PROCESSED_EVENT, [this](const gringofts::Event &event) -> gringofts::StateMachine & {
    return this->apply(dynamic_cast<const ProcessedEvent&>(event));
  });
}

ProcessHint AppStateMachine::processCommandAndApply(
    const Command &command, std::vector<std::shared_ptr<Event>> *events) {
  auto hint = processCommand(command, events);
  if (events->empty()) {
    return hint;
  }

  for (const auto &event : *events) {
    applyEvent(*event);
  }
  return hint;
}
}  // namespace gringofts::demo

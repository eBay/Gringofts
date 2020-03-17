/************************************************************************
Copyright 2019-2020 eBay Inc.
Authors/Developers: Bing (Glen) Gen, Qi (Jacky) Jia
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

#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_COMMANDPROCESSLOOP_HPP_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_COMMANDPROCESSLOOP_HPP_

namespace gringofts {
namespace demo {

template<typename StateMachineType>
void CommandProcessLoop<StateMachineType>::processCommand(std::shared_ptr<Command> command) {
  /// in case no events generated
  command->setId(this->mLastCommandId);

  auto ts1InNano = TimeUtil::currentTimeInNanos();
  std::vector<std::shared_ptr<Event>> events;
  auto hint = this->mAppStateMachine->processCommandAndApply(*command, &events);

  if (events.empty()) {
    SPDLOG_WARN("Error processing command {}, error code: {}, error message: {}",
                command->getId(),
                hint.mCode,
                hint.mMessage);
    this->mCommandEventStore->persistAsync(command, {}, hint.mCode, hint.mMessage);
    return;
  }
  auto ts2InNano = TimeUtil::currentTimeInNanos();

  /// update metrics
  this->applied_event_total.increase(events.size());
  this->processed_command_total.increase();

  /// reset commandId
  command->setId(++this->mLastCommandId);
  this->mCommandEventStore->persistAsync(command, events, hint.mCode, hint.mMessage);

  auto ts3InNano = TimeUtil::currentTimeInNanos();

  SPDLOG_INFO("Process Command {}, applied {} events, "
              "processCost={}us, persistCost={}us",
              command->getId(), events.size(),
              (ts2InNano - ts1InNano) / 1000.0,
              (ts3InNano - ts2InNano) / 1000.0);
}

}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_COMMANDPROCESSLOOP_HPP_

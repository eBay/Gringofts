/************************************************************************
Copyright 2019-2021 eBay Inc.
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

#ifndef SRC_APP_UTIL_APPSTATEMACHINE_H_
#define SRC_APP_UTIL_APPSTATEMACHINE_H_

#include "../infra/es/ProcessCommandStateMachine.h"
#include "control/CtrlState.h"
#include "control/CtrlCommandEvent.h"
#include "control/split/SplitCommand.h"
#include "control/split/SplitEvent.h"

namespace gringofts::app {

using gringofts::app::ctrl::CtrlState;

class AppStateMachine : public gringofts::ProcessCommandStateMachine {
 public:
  AppStateMachine() = default;

  // disallow copy ctor and copy assignment
  AppStateMachine(const AppStateMachine &) = delete;
  AppStateMachine &operator=(const AppStateMachine &) = delete;

  // disallow move ctor and move assignment
  AppStateMachine(AppStateMachine &&) = delete;
  AppStateMachine &operator=(AppStateMachine &&) = delete;

  using CommandProcessor = std::function<ProcessHint(
      const Command &,
      std::vector<std::shared_ptr<Event>> *)>;

  using EventApplier = std::function<StateMachine &(const Event &)>;  // NOLINT [runtime/casting]

  ProcessHint processCommand(
      const Command &command,
      std::vector<std::shared_ptr<Event>> *events) const override {
    if (ctrl::isCtrlCommand(command.getType())) {
      SPDLOG_INFO("ctrl command is processing");
      return dynamic_cast<const ctrl::CtrlCommand &>(command).process(mCtrlState, events);
    }
    const auto entry = mCommandProcessorMap.find(command.getType());
    if (entry != mCommandProcessorMap.end()) {
      uint64_t code;
      std::string errMsg;
      if (!checkCommand(command, mCtrlState, &code, &errMsg)) {
        return ProcessHint{code, "forbidden access for " + errMsg};
      }
      CommandProcessor commandProcessor = entry->second;
      return commandProcessor(command, events);
    }
    SPDLOG_WARN("Cannot find command processor for {}", command.getType());
    return ProcessHint{503, "Cannot find command processor for " + std::to_string(command.getType())};
  }

  StateMachine &applyEvent(const Event &event) override {
    if (ctrl::isCtrlEvent(event.getType())) {
      SPDLOG_INFO("ctrl event is applying");
      dynamic_cast<const ctrl::CtrlEvent &>(event).apply(&mCtrlState);
      setCtrlState(mCtrlState);
      return *this;
    }
    const auto entry = mEventApplierMap.find(event.getType());
    if (entry != mEventApplierMap.end()) {
      EventApplier eventApplier = entry->second;
      return eventApplier(event);
    }
    SPDLOG_WARN("Cannot find event applier for {}", event.getType());
    return *this;
  }

  const CtrlState &ctrlState() const { return mCtrlState; }

 protected:
  virtual void setCtrlState(const CtrlState &state) {}

  virtual bool checkCommand(const Command &command, const CtrlState &state, uint64_t *code, std::string *errMsg) const {
    return true;
  }

  void registerCommandProcessor(Type commandTypeId, const CommandProcessor &commandProcessor) {
    mCommandProcessorMap.emplace(commandTypeId, commandProcessor);
  }

  void registerEventApplier(Type eventTypeId, const EventApplier &eventApplier) {
    mEventApplierMap.emplace(eventTypeId, eventApplier);
  }

 protected:
  CtrlState mCtrlState;

 private:
  std::unordered_map<Type, CommandProcessor> mCommandProcessorMap;
  std::unordered_map<Type, EventApplier> mEventApplierMap;
};

}  // namespace gringofts::app

#endif  // SRC_APP_UTIL_APPSTATEMACHINE_H_

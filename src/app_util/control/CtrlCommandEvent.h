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

#ifndef SRC_APP_UTIL_CONTROL_CTRLCOMMANDEVENT_H_
#define SRC_APP_UTIL_CONTROL_CTRLCOMMANDEVENT_H_

#include "../../infra/common_types.h"
#include "../../infra/es/Command.h"
#include "../../infra/es/Event.h"
#include "../../infra/es/ProcessCommandStateMachine.h"
#include "CtrlState.h"

namespace gringofts::app::ctrl {

const Type CTRL_TYPES = 100;

inline bool isCtrlCommand(Type commandType) { return commandType > CTRL_TYPES; }
inline bool isCtrlEvent(Type eventType) { return eventType > CTRL_TYPES; }

class CtrlCommand : public Command {
 public:
  CtrlCommand(Type type, TimestampInNanos createdTimeInNanos)
      : Command(type, createdTimeInNanos) {}
  virtual ProcessHint process(const CtrlState &state,
                              std::vector<std::shared_ptr<Event>> *events) const = 0;

 protected:
  void onPersisted(const std::string &message) override {
    auto *callData = getRequestHandle();
    if (callData == nullptr) {
      SPDLOG_WARN("This command does not have request attached.");
      return;
    }
    callData->fillResultAndReply(200, message, std::nullopt);
  }

  void onPersistFailed(uint32_t code, const std::string &errorMessage, std::optional<uint64_t> reserved) override {
    auto *callData = getRequestHandle();
    if (callData == nullptr) {
      SPDLOG_WARN("This command does not have request attached.");
      return;
    }
    callData->fillResultAndReply(503, errorMessage, reserved);
  }
};

class CtrlEvent : public Event {
 public:
  CtrlEvent(Type type, TimestampInNanos createdTimeInNanos)
      : Event(type, createdTimeInNanos) {}
  virtual void apply(CtrlState *State) const = 0;
};

}  // namespace gringofts::app::ctrl

#endif  // SRC_APP_UTIL_CONTROL_CTRLCOMMANDEVENT_H_

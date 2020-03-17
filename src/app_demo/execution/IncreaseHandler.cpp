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

#include "IncreaseHandler.h"

#include <spdlog/spdlog.h>

#include "../../app_util/AppInfo.h"
#include "../AppStateMachine.h"

namespace gringofts {
namespace demo {

ProcessHint IncreaseHandler::process(const AppStateMachine &appStateMachine,
                                     const IncreaseCommand &increaseCommand,
                                     std::vector<std::shared_ptr<Event>> *events) {
  if (appStateMachine.getValue() >= increaseCommand.getValue()) {
    return ProcessHint{201, "Duplicated request"};
  }
  if (appStateMachine.getValue() + 1 < increaseCommand.getValue()) {
    return ProcessHint{400, "Invalid request"};
  }

  events->push_back(std::make_shared<ProcessedEvent>(TimeUtil::currentTimeInNanos(), increaseCommand.getRequest()));

  for (auto &eventPtr : *events) {
    eventPtr->setCreatorId(app::AppInfo::subsystemId());
    eventPtr->setGroupId(app::AppInfo::groupId());
    eventPtr->setGroupVersion(app::AppInfo::groupVersion());
  }

  return ProcessHint{200, "Success"};
}

}  /// namespace demo
}  /// namespace gringofts

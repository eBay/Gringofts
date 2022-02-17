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

#ifndef SRC_APP_DEMO_EXECUTION_INCREASEHANDLER_H_
#define SRC_APP_DEMO_EXECUTION_INCREASEHANDLER_H_

#include <string>

#include "../../app_util/AppInfo.h"
#include "../../infra/es/Event.h"
#include "../should_be_generated/domain/IncreaseCommand.h"

namespace gringofts {
namespace demo {

class AppStateMachine;

class IncreaseHandler {
 public:
  explicit IncreaseHandler(const app::AppInfo &appInfo) : mAppInfo(appInfo) {}
  ProcessHint process(const AppStateMachine &appStateMachine,
                      const IncreaseCommand &increaseCommand,
                      std::vector<std::shared_ptr<Event>> *);
 private:
  const app::AppInfo &mAppInfo;
};

}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_EXECUTION_INCREASEHANDLER_H_

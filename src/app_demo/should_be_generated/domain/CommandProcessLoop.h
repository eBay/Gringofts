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

#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_COMMANDPROCESSLOOP_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_COMMANDPROCESSLOOP_H_

#include "../../../app_util/CommandProcessLoop.h"
#include "../../v2/MemoryBackedAppStateMachine.h"

namespace gringofts {
namespace demo {

template<typename StateMachineType>
class CommandProcessLoop : public app::CommandProcessLoop<StateMachineType> {
 public:
  using app::CommandProcessLoop<StateMachineType>::CommandProcessLoop;

 private:
  void processCommand(std::shared_ptr<Command>) override;
};

}  /// namespace demo
}  /// namespace gringofts

#include "CommandProcessLoop.hpp"

#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_COMMANDPROCESSLOOP_H_

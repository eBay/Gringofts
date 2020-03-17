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

#ifndef SRC_INFRA_ES_STORE_DEFAULTCOMMANDEVENTSTORE_H_
#define SRC_INFRA_ES_STORE_DEFAULTCOMMANDEVENTSTORE_H_

#include "../CommandEventStore.h"

namespace gringofts {

/**
 * The default (dummy) implementation of #gringofts::CommandEventStore which doesn't
 * persist anything.
 * This is mainly used as a placeholder, and can also be used when doing
 * performance analysis extracting the factor of persistence.
 */
class DefaultCommandEventStore final : public CommandEventStore {
 public:
  void persistAsync(const std::shared_ptr<Command> &command,
                    const std::vector<std::shared_ptr<Event>> &events,
                    uint64_t code,
                    const std::string &message) override {
    SPDLOG_WARN("dummy method, command and events are NOT persisted. Please use other store.");
    command->onPersisted();
  }

  void run() override {
    SPDLOG_WARN("dummy method, does nothing. Please use other store.");
  }

  void shutdown() override {
    SPDLOG_WARN("dummy method, does nothing. Please use other store.");
  }
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_STORE_DEFAULTCOMMANDEVENTSTORE_H_

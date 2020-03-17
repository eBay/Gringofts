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

#ifndef SRC_INFRA_ES_STORE_READONLYDEFAULTCOMMANDEVENTSTORE_H_
#define SRC_INFRA_ES_STORE_READONLYDEFAULTCOMMANDEVENTSTORE_H_

#include "../ReadonlyCommandEventStore.h"

namespace gringofts {

class ReadonlyDefaultCommandEventStore : public ReadonlyCommandEventStore {
 public:
  std::unique_ptr<Event> loadNextEvent(const EventDecoder &) override {
    SPDLOG_ERROR("dummy method, always return null. Please use other store.");
    return nullptr;
  }

  std::unique_ptr<Command> loadCommandAfter(Id, const CommandDecoder &) override {
    SPDLOG_ERROR("dummy method, always return null. Please use other store.");
    return nullptr;
  }

  std::unique_ptr<Command> loadNextCommand(const CommandDecoder &) override {
    SPDLOG_ERROR("dummy method, always return null. Please use other store.");
    return nullptr;
  }

  CommandEventsOpt loadNextCommandEvents(const CommandDecoder &, const EventDecoder &) override {
    SPDLOG_ERROR("dummy method, always return null. Please use other store.");
    return std::nullopt;
  }

  uint64_t loadCommandEventsList(const CommandDecoder &,
                                 const EventDecoder &,
                                 Id,
                                 uint64_t,
                                 CommandEventsList *) override {
    SPDLOG_WARN("dummy method, always return empty. Please use other store.");
    return 0;
  }

  uint64_t getCurrentOffset() const override {
    SPDLOG_ERROR("dummy method, always return 0. Please use other store.");
    return 0;
  }

  void setCurrentOffset(uint64_t) override {
    SPDLOG_ERROR("dummy method. Please use other store.");
  }

  void truncatePrefix(uint64_t) override {
    SPDLOG_WARN("dummy method. Please use other store.");
  }
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_STORE_READONLYDEFAULTCOMMANDEVENTSTORE_H_

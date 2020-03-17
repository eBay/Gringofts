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

#ifndef SRC_INFRA_ES_STORE_READONLYSQLITECOMMANDEVENTSTORE_H_
#define SRC_INFRA_ES_STORE_READONLYSQLITECOMMANDEVENTSTORE_H_

#include <spdlog/spdlog.h>

#include "../../util/IdGenerator.h"
#include "../ReadonlyCommandEventStore.h"
#include "SQLiteStoreDao.h"

namespace gringofts {

class ReadonlySQLiteCommandEventStore : public ReadonlyCommandEventStore {
 public:
  explicit ReadonlySQLiteCommandEventStore(std::shared_ptr<SQLiteStoreDao>,
                                           bool,
                                           std::shared_ptr<IdGenerator>,
                                           std::shared_ptr<IdGenerator>);

  std::unique_ptr<Event> loadNextEvent(const EventDecoder &) override;

  std::unique_ptr<Command> loadCommandAfter(Id, const CommandDecoder &) override;

  std::unique_ptr<Command> loadNextCommand(const CommandDecoder &) override;

  CommandEventsOpt loadNextCommandEvents(const CommandDecoder &, const EventDecoder &) override;

  uint64_t loadCommandEventsList(const CommandDecoder &,
                                 const EventDecoder &,
                                 Id,
                                 uint64_t,
                                 CommandEventsList *) override;

  uint64_t getCurrentOffset() const override {
    return mCurrentLoadedCommandOffset;
  }

  void setCurrentOffset(uint64_t currentOffset) override {
    mCurrentLoadedCommandOffset = currentOffset;
  }

  void truncatePrefix(uint64_t offsetKept) override {
    SPDLOG_WARN("nothing is going to happen on SQLite Store");
  }

 private:
  std::shared_ptr<SQLiteStoreDao> mSqliteStoreDao;

  Id mCurrentLoadedEventOffset = 0;  // event id starts from 1

  Id mCurrentLoadedCommandOffset = 0;  // command id starts from 1

  // true if it needs to update
  bool mNeedUpdateId = false;
  std::shared_ptr<IdGenerator> mCommandIdGenerator = nullptr;
  std::shared_ptr<IdGenerator> mEventIdGenerator = nullptr;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_STORE_READONLYSQLITECOMMANDEVENTSTORE_H_

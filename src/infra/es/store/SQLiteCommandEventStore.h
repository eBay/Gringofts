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

#ifndef SRC_INFRA_ES_STORE_SQLITECOMMANDEVENTSTORE_H_
#define SRC_INFRA_ES_STORE_SQLITECOMMANDEVENTSTORE_H_

#include <sqlite3.h>

#include "../../util/IdGenerator.h"
#include "../CommandEventStore.h"
#include "SQLiteStoreDao.h"

namespace gringofts {
/**
 * An implementation of #gringofts::CommandEventStore backed by SQLite
 * This is mainly used in unit test, covering persist and replay.
 */
class SQLiteCommandEventStore final : public CommandEventStore {
 public:
  SQLiteCommandEventStore(std::shared_ptr<SQLiteStoreDao>,
                          std::shared_ptr<IdGenerator>,
                          std::shared_ptr<IdGenerator>);

  void persistAsync(const std::shared_ptr<Command> &,
                    const std::vector<std::shared_ptr<Event>> &,
                    uint64_t,
                    const std::string &) override;

  void run() override;

  void shutdown() override;

 private:
  /**
   * Comparing with CommandEventStore::persistAsync, this method will block until all commands and events are persisted.
   * @param commands all commands in the batch
   * @param events all events in the batch, these events are the result of processing \p commands
   */
  void persist(const std::vector<std::shared_ptr<Command>> &commands,
               const std::vector<std::shared_ptr<Event>> &events);

 private:
  std::shared_ptr<SQLiteStoreDao> mSqliteStoreDao;

  CommandEventQueue mCommandEventQueue;

  std::atomic<bool> mStarted = false;
  std::atomic<bool> mShouldExit = false;

  std::shared_ptr<IdGenerator> mCommandIdGenerator;
  std::shared_ptr<IdGenerator> mEventIdGenerator;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_STORE_SQLITECOMMANDEVENTSTORE_H_

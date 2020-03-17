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

#ifndef SRC_INFRA_ES_STORE_SQLITESTOREDAO_H_
#define SRC_INFRA_ES_STORE_SQLITESTOREDAO_H_

#include <sqlite3.h>

#include "../../common_types.h"
#include "../Command.h"
#include "../CommandDecoder.h"
#include "../Event.h"
#include "../EventDecoder.h"

namespace gringofts {

/**
 * DAO for SQLite command event store
 */
class SQLiteStoreDao {
 public:
  explicit SQLiteStoreDao(const char *);
  ~SQLiteStoreDao();

  // disallow copy ctor and copy assignment
  SQLiteStoreDao(const SQLiteStoreDao &) = delete;
  SQLiteStoreDao &operator=(const SQLiteStoreDao &) = delete;

  // disallow move ctor and move assignment
  SQLiteStoreDao(SQLiteStoreDao &&) = delete;
  SQLiteStoreDao &operator=(SQLiteStoreDao &&) = delete;

 private:
  void handleResultCode(int, std::shared_ptr<Command>) const;

  void persist(const std::vector<std::shared_ptr<Event>> &);
  void persist(const std::vector<std::shared_ptr<Command>> &);

 public:
  void persist(const std::vector<std::shared_ptr<Command>> &,
               const std::vector<std::shared_ptr<Event>> &);

  std::unique_ptr<Event> findNextEvent(const Id &, const EventDecoder &) const;

  std::unique_ptr<Command> findNextCommand(const Id &, const CommandDecoder &) const;

  std::list<std::unique_ptr<Event>> getEventsByCommandId(const Id &, const EventDecoder &) const;

 private:
  std::unique_ptr<Command> populateCommand(sqlite3_stmt *, const CommandDecoder &) const;
  std::unique_ptr<Event> populateEvent(sqlite3_stmt *, const EventDecoder &) const;

  sqlite3 *mDb;
  sqlite3_stmt *mInsertCommandStmt;
  sqlite3_stmt *mSelectNextCommandStmt;
  sqlite3_stmt *mInsertEventStmt;
  sqlite3_stmt *mSelectNextEventStmt;
  sqlite3_stmt *mSelectEventsByCommandIdStmt;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_STORE_SQLITESTOREDAO_H_

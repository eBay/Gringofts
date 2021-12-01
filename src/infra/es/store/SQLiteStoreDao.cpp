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

#include "SQLiteStoreDao.h"

#include <assert.h>

#include <spdlog/spdlog.h>

#include "../../util/FileUtil.h"

namespace gringofts {

SQLiteStoreDao::SQLiteStoreDao(const char *pathToDbFile) {
  if (!FileUtil::fileExists(pathToDbFile)) throw std::runtime_error("Unable to open file");
  sqlite3_open(pathToDbFile, &mDb);
  sqlite3_prepare_v2(
      mDb,
      "INSERT INTO COMMAND (ID, TYPE, CREATEDTIME, CREATORID, GROUPID, GROUPVERSION, PAYLOAD) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);",
      -1,
      &mInsertCommandStmt,
      NULL);
  sqlite3_prepare_v2(
      mDb,
      "SELECT ID, TYPE, CREATEDTIME, CREATORID, GROUPID, GROUPVERSION, PAYLOAD FROM COMMAND "
      "WHERE ID>?1 ORDER BY ID ASC LIMIT 1;",
      -1,
      &mSelectNextCommandStmt,
      NULL);
  sqlite3_prepare_v2(
      mDb,
      "INSERT INTO EVENT (ID, TYPE, COMMANDID, CREATEDTIME, CREATORID, GROUPID, GROUPVERSION, PAYLOAD) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);",
      -1,
      &mInsertEventStmt,
      NULL);
  sqlite3_prepare_v2(
      mDb,
      "SELECT ID, TYPE, COMMANDID, CREATEDTIME, CREATORID, GROUPID, GROUPVERSION, PAYLOAD FROM EVENT "
      "WHERE ID>?1 ORDER BY ID ASC LIMIT 1;",
      -1,
      &mSelectNextEventStmt,
      NULL);
  sqlite3_prepare_v2(
      mDb,
      "SELECT ID, TYPE, COMMANDID, CREATEDTIME, CREATORID, GROUPID, GROUPVERSION, PAYLOAD FROM EVENT "
      "WHERE COMMANDID=?1;",
      -1,
      &mSelectEventsByCommandIdStmt,
      NULL);
}

SQLiteStoreDao::~SQLiteStoreDao() {
  sqlite3_close(mDb);
}

void SQLiteStoreDao::handleResultCode(int rc, std::shared_ptr<Command> command) const {
  if (rc != SQLITE_OK && rc != SQLITE_DONE) {
    SPDLOG_WARN("Warning: {} ", sqlite3_errmsg(mDb));
    if (command != nullptr)
      command->onPersistFailed(503, sqlite3_errmsg(mDb), std::nullopt);
    throw std::runtime_error("Error: Failed on save or query, possible reasons: 1. table doesn't exist; "
                             "2. failed on constraint.");
  } else {
    if (command != nullptr)
      command->onPersisted();
  }
}

void SQLiteStoreDao::persist(
    const std::vector<std::shared_ptr<Command>> &commands,
    const std::vector<std::shared_ptr<Event>> &events) {
  if (sqlite3_exec(mDb, "BEGIN", 0, 0, 0) != SQLITE_OK) {
    throw std::runtime_error("Cannot persist commands and events, error when starting transaction");
  }
  /// persist events first to make sure they'll be loaded
  /// when loadNextCommandEvents is called
  /// refer to https://www.sqlite.org/isolation.html
  /// "A query sees all changes that are completed on the same database connection prior to the start of the query,
  /// regardless of whether or not those changes have been committed."
  persist(events);
  persist(commands);
  if (sqlite3_exec(mDb, "COMMIT", 0, 0, 0) != SQLITE_OK) {
    throw std::runtime_error("Cannot persist commands and events, error when committing transaction");
  }
}

void SQLiteStoreDao::persist(const std::vector<std::shared_ptr<Command>> &commands) {
  for (const auto &command : commands) {
    assert(command->getId() > 0);
    const auto &commandString = command->encodeToString();
    SPDLOG_INFO("Persist Command size: {} ", commandString.size());
    sqlite3_reset(mInsertCommandStmt);
    sqlite3_bind_int64(mInsertCommandStmt, 1, command->getId());
    sqlite3_bind_int(mInsertCommandStmt, 2, command->getType());
    sqlite3_bind_int64(mInsertCommandStmt, 3, command->getCreatedTimeInNanos());
    sqlite3_bind_int64(mInsertCommandStmt, 4, command->getCreatorId());
    sqlite3_bind_int64(mInsertCommandStmt, 5, command->getGroupId());
    sqlite3_bind_int64(mInsertCommandStmt, 6, command->getGroupVersion());
    sqlite3_bind_text(mInsertCommandStmt, 7, commandString.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(mInsertCommandStmt);
    handleResultCode(rc, command);
  }
}

void SQLiteStoreDao::persist(const std::vector<std::shared_ptr<Event>> &events) {
  for (const auto &event : events) {
    assert(event->getId() > 0);
    assert(event->getCommandId() > 0);
    const auto &eventString = event->encodeToString();
    SPDLOG_INFO("Persist Event size: {} ", eventString.size());
    sqlite3_reset(mInsertEventStmt);
    sqlite3_bind_int64(mInsertEventStmt, 1, event->getId());
    sqlite3_bind_int(mInsertEventStmt, 2, event->getType());
    sqlite3_bind_int64(mInsertEventStmt, 3, event->getCommandId());
    sqlite3_bind_int64(mInsertEventStmt, 4, event->getCreatedTimeInNanos());
    sqlite3_bind_int64(mInsertEventStmt, 5, event->getCreatorId());
    sqlite3_bind_int64(mInsertEventStmt, 6, event->getGroupId());
    sqlite3_bind_int64(mInsertEventStmt, 7, event->getGroupVersion());
    sqlite3_bind_text(mInsertEventStmt, 8, eventString.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(mInsertEventStmt);
    handleResultCode(rc, nullptr);
  }
}

std::unique_ptr<Event> SQLiteStoreDao::findNextEvent(
    const Id &id, const EventDecoder &eventDecoder) const {
  if (id >= 0) {
    sqlite3_reset(mSelectNextEventStmt);
    sqlite3_bind_int64(mSelectNextEventStmt, 1, id);
    int rc = sqlite3_step(mSelectNextEventStmt);
    if (rc == SQLITE_ROW) {
      return populateEvent(mSelectNextEventStmt, eventDecoder);
    } else {
      handleResultCode(rc, nullptr);
      return nullptr;
    }
  } else {
    return nullptr;
  }
}

std::unique_ptr<Command> SQLiteStoreDao::findNextCommand(
    const Id &id, const CommandDecoder &commandDecoder) const {
  if (id >= 0) {
    sqlite3_reset(mSelectNextCommandStmt);
    sqlite3_bind_int64(mSelectNextCommandStmt, 1, id);
    int rc = sqlite3_step(mSelectNextCommandStmt);
    if (rc == SQLITE_ROW) {
      return populateCommand(mSelectNextCommandStmt, commandDecoder);
    } else {
      handleResultCode(rc, nullptr);
      return nullptr;
    }
  } else {
    return nullptr;
  }
}

std::list<std::unique_ptr<Event>> SQLiteStoreDao::getEventsByCommandId(
    const gringofts::Id &id,
    const gringofts::EventDecoder &eventDecoder) const {
  std::list<std::unique_ptr<Event>> events;
  if (id >= 0) {
    sqlite3_reset(mSelectEventsByCommandIdStmt);
    sqlite3_bind_int64(mSelectEventsByCommandIdStmt, 1, id);
    int rc = sqlite3_step(mSelectEventsByCommandIdStmt);
    while (rc != SQLITE_DONE) {
      if (rc == SQLITE_ROW) {
        events.push_back(populateEvent(mSelectEventsByCommandIdStmt, eventDecoder));
      } else {
        handleResultCode(rc, nullptr);
      }
      rc = sqlite3_step(mSelectEventsByCommandIdStmt);
    }
  }

  return std::move(events);
}

std::unique_ptr<Event> SQLiteStoreDao::populateEvent(sqlite3_stmt *selectEventStmt,
                                                     const EventDecoder &eventDecoder) const {
  const auto &eventId = sqlite3_column_int64(selectEventStmt, 0);
  const auto &type = sqlite3_column_int(selectEventStmt, 1);
  const auto &commandId = sqlite3_column_int64(selectEventStmt, 2);
  const auto &createdTimeInNanos = sqlite3_column_int64(selectEventStmt, 3);
  const auto &creatorId = sqlite3_column_int64(selectEventStmt, 4);
  const auto &groupId = sqlite3_column_int64(selectEventStmt, 5);
  const auto &groupVersion = sqlite3_column_int64(selectEventStmt, 6);
  const auto &payload = (const char *) sqlite3_column_text(selectEventStmt, 7);
  // defensive copy payload as the returned pointer may NOT be valid when it is being processed.
  // see https://www.sqlite.org/c3ref/column_blob.html
  // "The pointers returned are valid until a type conversion occurs as described above,
  // or until sqlite3_step() or sqlite3_reset() or sqlite3_finalize() is called.
  // The memory space used to hold strings and BLOBs is freed automatically."
  EventMetaData metaData;
  metaData.setId(eventId);
  metaData.setType(Type(type));
  metaData.setCommandId(commandId);
  metaData.setCreatedTimeInNanos(createdTimeInNanos);
  metaData.setCreatorId(creatorId);
  metaData.setGroupId(groupId);
  metaData.setGroupVersion(groupVersion);
  return eventDecoder.decodeEventFromString(metaData, std::string(payload).c_str());
}

std::unique_ptr<Command> SQLiteStoreDao::populateCommand(sqlite3_stmt *selectCommandStmt,
                                                         const CommandDecoder &commandDecoder) const {
  const auto &commandId = sqlite3_column_int64(selectCommandStmt, 0);
  const auto &type = sqlite3_column_int(selectCommandStmt, 1);
  const auto &createdTimeInNanos = sqlite3_column_int64(selectCommandStmt, 2);
  const auto &creatorId = sqlite3_column_int64(selectCommandStmt, 3);
  const auto &groupId = sqlite3_column_int64(selectCommandStmt, 4);
  const auto &groupVersion = sqlite3_column_int64(selectCommandStmt, 5);
  const auto &payload = (const char *) sqlite3_column_text(selectCommandStmt, 6);
  // defensive copy payload as the returned pointer may NOT be valid when it is being processed.
  // see https://www.sqlite.org/c3ref/column_blob.html
  // "The pointers returned are valid until a type conversion occurs as described above,
  // or until sqlite3_step() or sqlite3_reset() or sqlite3_finalize() is called.
  // The memory space used to hold strings and BLOBs is freed automatically."
  CommandMetaData metaData;
  metaData.setId(commandId);
  metaData.setType(Type(type));
  metaData.setCreatedTimeInNanos(createdTimeInNanos);
  metaData.setCreatorId(creatorId);
  metaData.setGroupId(groupId);
  metaData.setGroupVersion(groupVersion);
  return commandDecoder.decodeCommandFromString(metaData, std::string(payload).c_str());
}

}  /// namespace gringofts

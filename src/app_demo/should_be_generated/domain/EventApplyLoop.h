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

#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_EVENTAPPLYLOOP_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_EVENTAPPLYLOOP_H_

#include "../../v2/RocksDBBackedAppStateMachine.h"
#include "../../../app_util/EventApplyLoop.h"

namespace gringofts {
namespace app {

/**
 * Full specification for demo::v2::AppStateMachine, which is based on RocksDB
 */
template<> inline std::optional<uint64_t>
EventApplyLoop<demo::v2::AppStateMachine>::getLatestSnapshotOffset() const {
  SPDLOG_WARN("StateMachine v2 based on RocksDB does not support snapshot.");
  return std::nullopt;
}

template<> inline std::pair<bool, std::string>
EventApplyLoop<demo::v2::AppStateMachine>::takeSnapshotAndPersist() const {
  SPDLOG_WARN("StateMachine v2 based on RocksDB does not support snapshot.");
  return std::make_pair(false, "");
}

template<> inline void
EventApplyLoop<demo::v2::AppStateMachine>::initStateMachine(const INIReader &iniReader) {
  std::string walDir = iniReader.Get("rocksdb", "wal.dir", "");
  std::string dbDir  = iniReader.Get("rocksdb", "db.dir", "");
  assert(!walDir.empty() && !dbDir.empty());

  mAppStateMachine = std::make_unique<demo::v2::RocksDBBackedAppStateMachine>(walDir, dbDir);
}

template<> inline void
EventApplyLoop<demo::v2::AppStateMachine>::recoverSelf() {
  SPDLOG_INFO("Start recovering.");

  /// recover StateMachine
  mLastAppliedLogEntryIndex = mAppStateMachine->recoverSelf();

  /// re-init Readonly CES, unit test CAN ignore this step.
  if (mReadonlyCommandEventStore) {
    mReadonlyCommandEventStore->setCurrentOffset(mLastAppliedLogEntryIndex);
    mReadonlyCommandEventStore->init();
  }

  mShouldRecover = false;
}

}  /// namespace app
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_EVENTAPPLYLOOP_H_

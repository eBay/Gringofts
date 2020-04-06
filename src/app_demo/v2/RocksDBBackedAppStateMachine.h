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

#ifndef SRC_APP_DEMO_V2_ROCKSDBBACKEDAPPSTATEMACHINE_H_
#define SRC_APP_DEMO_V2_ROCKSDBBACKEDAPPSTATEMACHINE_H_

#include <rocksdb/db.h>
#include <rocksdb/options.h>

#include "AppStateMachine.h"

namespace gringofts {
namespace demo {
namespace v2 {

class RocksDBBackedAppStateMachine : public v2::AppStateMachine {
 public:
  RocksDBBackedAppStateMachine(const std::string &walDir, const std::string &dbDir)
  { openRocksDB(walDir, dbDir, &mRocksDB); }

  ~RocksDBBackedAppStateMachine() override { closeRocksDB(&mRocksDB); }

  /**
   * implement getter() and setter()
   */
  uint64_t getValue() const override;
  void setValue(uint64_t value) override;

  /**
   * integration part
   */
  void swapState(StateMachine *anotherStateMachine) override { assert(0); }

  /// invoked after swapState() is called, return lastFlushedIndex
  uint64_t recoverSelf();

  /// call flushToRocksDB() if needed.
  void commit(uint64_t appliedIndex) override;

  std::string createCheckpoint(const std::string &baseDir) { assert(0); }

 private:
  friend class MemoryBackedAppStateMachine;

  /// open RocksDB
  void openRocksDB(const std::string &walDir,
                   const std::string &dbDir,
                   std::shared_ptr<rocksdb::DB> *dbPtr);

  /// close RocksDB
  void closeRocksDB(std::shared_ptr<rocksdb::DB> *dbPtr);

  /// write WriteBatch to RocksDB synchronously
  void flushToRocksDB();

  /// read value/lastAppliedIndex from RocksDB
  void loadFromRocksDB();

  /// the max num of bundles batched in write batch
  const uint64_t mMaxBatchSize = 5;

  std::shared_ptr<rocksdb::DB> mRocksDB;
  rocksdb::WriteBatch mWriteBatch;

  /// latest index that have been flushed to RocksDB
  uint64_t mLastFlushedIndex = 0;
};

}  /// namespace v2
}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_V2_ROCKSDBBACKEDAPPSTATEMACHINE_H_

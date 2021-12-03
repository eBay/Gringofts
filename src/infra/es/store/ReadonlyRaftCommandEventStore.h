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

#ifndef SRC_INFRA_ES_STORE_READONLYRAFTCOMMANDEVENTSTORE_H_
#define SRC_INFRA_ES_STORE_READONLYRAFTCOMMANDEVENTSTORE_H_

#include <atomic>
#include <shared_mutex>

#include "../../raft/RaftInterface.h"
#include "../../util/CryptoUtil.h"
#include "../ReadonlyCommandEventStore.h"

namespace gringofts {

class ReadonlyRaftCommandEventStore final : public ReadonlyCommandEventStore {
 public:
  ReadonlyRaftCommandEventStore(const std::shared_ptr<raft::RaftInterface> &,
                                const std::shared_ptr<CommandDecoder> &,
                                const std::shared_ptr<EventDecoder> &,
                                const std::shared_ptr<CryptoUtil> &,
                                bool asyncLoad);

  ~ReadonlyRaftCommandEventStore() override { teardown(); }

  void init() override;
  void teardown() override;

  std::unique_ptr<Command> loadCommandAfter(Id, const CommandDecoder &) override {
    throw std::runtime_error("Not supported");
  }

  std::unique_ptr<Command> loadNextCommand(const CommandDecoder &) override {
    throw std::runtime_error("Not supported");
  }

  std::unique_ptr<Event> loadNextEvent(const EventDecoder &) override;
  CommandEventsOpt loadNextCommandEvents(const CommandDecoder &, const EventDecoder &) override;

  uint64_t loadCommandEventsList(const CommandDecoder &,
                                 const EventDecoder &,
                                 Id,
                                 uint64_t,
                                 CommandEventsList *) override;

  /**
   * wait until leader is ready for expected term or step down.
   *
   * Here is the logic:
   * 1) currentTerm == expectedTerm
   * 2) loadedIndex == commitIndex == lastIndex
   * 3) entry between [appliedIndex + 1, lastIndex - 1] are all no-ops
   * 4) entry at lastIndex is no-op, its logTerm is expectedTerm.
   */
  uint64_t waitTillLeaderIsReadyOrStepDown(uint64_t expectedTerm) const override;
  bool isLeader() const override;

  /// only used by publisher and will be deprecated once pull-mode downstream is enabled
  uint64_t getCurrentOffset() const override { return mAppliedIndex; }

  void setCurrentOffset(uint64_t currentOffset) override {
    assert(mAppliedIndex == 0 && mLoadedIndex == 0);
    mAppliedIndex = currentOffset;
    mLoadedIndex = currentOffset;
  }

  void truncatePrefix(uint64_t offsetKept) override { mRaftImpl->truncatePrefix(offsetKept); }

  /**
 * Is syncing log
 */
  bool isSyncing() const override {
    return mRaftImpl->getRaftRole() == raft::RaftRole::Syncer;
  }

  /**
   * the first index we can fetch data
   */
  uint64_t firstIndex() const override {
    return mRaftImpl->getFirstLogIndex();
  }

  /**
   * The begin index of this cluster
   */
  uint64_t beginIndex() const override {
    return mRaftImpl->getBeginLogIndex();
  }

 private:
  /// decrypt entries to bundles
  void decryptEntries(std::vector<raft::LogEntry> *entries,
                      std::list<CommandEvents> *bundles);

  /// 1) load committed entries between [startIndex, startIndex + size - 1]
  /// 2) decode entries to bundles
  /// 3) return real fetched entry num.
  uint64_t loadBundles(uint64_t startIndex, uint64_t size,
                       std::list<CommandEvents> *bundles);

  /// thread function for mLoadThread
  void loadEntriesThreadMain();

  /// thread function for mDecryptThreads
  void decryptEntriesThreadMain();

  /// sync load, update mLoadedIndex and mCachedBundles if needed.
  void trySyncLoadBundles();

  /// async load, try to pop one bundles from mTaskQueue to mCachedBundles.
  void tryAsyncLoadBundles();

  void tryLoadBundles() {
    if (mAsyncLoad) {
      tryAsyncLoadBundles();
    } else {
      trySyncLoadBundles();
    }
  }

  std::shared_ptr<raft::RaftInterface> mRaftImpl;
  bool mAsyncLoad = false;

  /// max index of entry loaded from disk
  std::atomic<uint64_t> mLoadedIndex = 0;
  /// max index of applied entry
  std::atomic<uint64_t> mAppliedIndex = 0;

  /// index of applying entry
  uint64_t mApplyingIndex = 0;
  /// cached bundles
  std::list<CommandEvents> mCachedBundles;

  /// crypto and decoder
  std::shared_ptr<CommandDecoder> mCommandDecoder;
  std::shared_ptr<EventDecoder> mEventDecoder;
  std::shared_ptr<CryptoUtil> mCrypto;

  /**
   * optimize for sync load
   */
  const uint64_t kSpinLimit = 50;
  uint64_t mSpinTimes = 0;

  /**
   * optimize for async load
   */
  struct Task {
    std::vector<raft::LogEntry> entries;
    std::list<CommandEvents> bundles;

    /// 0:initial, 1:doing, 2:done
    std::atomic<uint64_t> flag = 0;
  };

  using TaskPtr = std::shared_ptr<Task>;

  /// structure of mTaskQueue is protected by mMutex, meanwhile, element is not.
  /// access mTaskQueue need read lock, modify (push/pop) mTaskQueue need unique lock
  std::list<TaskPtr> mTaskQueue;
  mutable std::shared_mutex mMutex;

  /// single load thread, call push_back() on mTaskQueue
  /// single apply thread (loop containing this store), call pop_front() on mTaskQueue
  /// multi decrypt thread, concurrently decrypt tasks
  std::thread mLoadThread;
  std::vector<std::thread> mDecryptThreads;
  std::atomic<bool> mRunning = true;

  /// configurable vars
  const uint64_t kMaxQueueSize = 30;
  const uint64_t kDecryptConcurrency = 6;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_STORE_READONLYRAFTCOMMANDEVENTSTORE_H_

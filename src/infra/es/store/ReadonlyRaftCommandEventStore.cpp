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

#include "ReadonlyRaftCommandEventStore.h"

#include "store.grpc.pb.h"
#include "CommandEventDecodeWrapper.h"

namespace {
using ::gringofts::es::CommandEntry;
using ::gringofts::es::EventEntry;
using ::gringofts::es::RaftPayload;
}

namespace gringofts {

ReadonlyRaftCommandEventStore::ReadonlyRaftCommandEventStore(const std::shared_ptr<raft::RaftInterface> &raftImpl,
                                                             const std::shared_ptr<CommandDecoder> &commandDecoder,
                                                             const std::shared_ptr<EventDecoder> &eventDecoder,
                                                             const std::shared_ptr<CryptoUtil> &crypto,
                                                             bool asyncLoad) :
    mRaftImpl(raftImpl),
    mCommandDecoder(commandDecoder),
    mEventDecoder(eventDecoder),
    mCrypto(crypto),
    mAsyncLoad(asyncLoad) {
  SPDLOG_INFO("ReadOnly mode: {}", mAsyncLoad ? "Async Load" : "Sync Load");
}

void ReadonlyRaftCommandEventStore::init() {
  uint64_t ts1InNano = TimeUtil::currentTimeInNanos();
  mRunning = true;

  if (!mAsyncLoad) {
    return;
  }

  mLoadThread = std::thread(&ReadonlyRaftCommandEventStore::loadEntriesThreadMain, this);

  for (std::size_t i = 0; i < kDecryptConcurrency; ++i) {
    mDecryptThreads.emplace_back(&ReadonlyRaftCommandEventStore::decryptEntriesThreadMain, this);
  }

  uint64_t ts2InNano = TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("Two phase initialize for CES cost {}ms", (ts2InNano - ts1InNano) / 1000000.0);
}

void ReadonlyRaftCommandEventStore::teardown() {
  uint64_t ts1InNano = TimeUtil::currentTimeInNanos();

  /// join threads
  mRunning = false;

  if (mLoadThread.joinable()) {
    mLoadThread.join();
  }

  for (auto &t : mDecryptThreads) {
    if (t.joinable()) {
      t.join();
    }
  }

  /// cleanup state
  mLoadedIndex = 0;
  mAppliedIndex = 0;
  mApplyingIndex = 0;

  mCachedBundles.clear();
  mTaskQueue.clear();

  mDecryptThreads.clear();

  uint64_t ts2InNano = TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("TearDown cost {}ms", (ts2InNano - ts1InNano) / 1000000.0);
}

/// only used by publisher and will be deprecated once pull-mode downstream is enabled
std::unique_ptr<Event>
ReadonlyRaftCommandEventStore::loadNextEvent(const EventDecoder &/*ignored*/) {
  if (mCachedBundles.empty()) {
    tryLoadBundles();
  }

  if (mCachedBundles.empty()) {
    /// command, events not ready yet
    return nullptr;
  }

  CommandEvents &bundle = mCachedBundles.front();
  auto &events = bundle.second;
  assert(!events.empty());
  auto event = std::move(events.front());
  events.pop_front();
  if (events.empty()) {
    mCachedBundles.pop_front();
  }

  return event;
}

ReadonlyRaftCommandEventStore::CommandEventsOpt
ReadonlyRaftCommandEventStore::loadNextCommandEvents(const CommandDecoder &, const EventDecoder &) {
  /// update applied
  mAppliedIndex = std::max(mAppliedIndex.load(), mApplyingIndex);

  if (mCachedBundles.empty()) {
    tryLoadBundles();
  }

  if (mCachedBundles.empty()) {
    /// command, events not ready yet
    return std::nullopt;
  }

  auto bundle = std::move(mCachedBundles.front());
  mCachedBundles.pop_front();

  /// update applying
  mApplyingIndex = bundle.first->getId();

  return CommandEventsOpt(std::move(bundle));
}

uint64_t
ReadonlyRaftCommandEventStore::loadCommandEventsList(const CommandDecoder &,
                                                     const EventDecoder &,
                                                     Id commandId,
                                                     uint64_t size,
                                                     CommandEventsList *bundles) {
  auto commitIndex = mRaftImpl->getCommitIndex();

  if (commandId <= 0 || commandId > commitIndex) {
    return 0;
  }

  if (commandId + size - 1 > commitIndex) {
    size = commitIndex - commandId + 1;
  }

  return loadBundles(commandId, size, bundles);
}

uint64_t ReadonlyRaftCommandEventStore::loadBundles(uint64_t startIndex, uint64_t size,
                                                    std::list<CommandEvents> *bundles) {
  uint64_t ts1InNano = TimeUtil::currentTimeInNanos();

  std::vector<raft::LogEntry> entries;
  size = mRaftImpl->getEntries(startIndex, size, &entries);

  uint64_t ts2InNano = TimeUtil::currentTimeInNanos();
  decryptEntries(&entries, bundles);

  uint64_t ts3InNano = TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("CommandEventStore Load {} Entry, totalCost={}ms, storageCost={}ms, decodeCost={}ms",
              size,
              (ts3InNano - ts1InNano) / 1000000.0,
              (ts2InNano - ts1InNano) / 1000000.0,
              (ts3InNano - ts2InNano) / 1000000.0);
  return size;
}

uint64_t ReadonlyRaftCommandEventStore::waitTillLeaderIsReadyOrStepDown(uint64_t expectedTerm) const {
  SPDLOG_INFO("Start waiting for expected term {}. "
              "The hint noop is <index,term>=<{},{}>",
              expectedTerm, mRaftImpl->getLastLogIndex(), expectedTerm);

  while (1) {
    /// step 1
    auto currentTerm = mRaftImpl->getCurrentTerm();
    if (currentTerm != expectedTerm) {
      SPDLOG_WARN("Leader Step Down, since currentTerm {} != expectedTerm {}",
                  currentTerm, expectedTerm);
      return 0;
    }

    /// step 2
    uint64_t lastIndex = mRaftImpl->getLastLogIndex();
    uint64_t commitIndex = mRaftImpl->getCommitIndex();
    uint64_t loadedIndex = mLoadedIndex;
    uint64_t appliedIndex = mAppliedIndex;

    if (loadedIndex != commitIndex || commitIndex != lastIndex) {
      continue;
    }

    /// step 3 and step 4
    for (auto index = appliedIndex + 1; index <= lastIndex; ++index) {
      raft::LogEntry entry;
      assert(mRaftImpl->getEntry(index, &entry));

      if (!entry.noop()) {
        break;
      }

      if (index == lastIndex && entry.term() == expectedTerm) {
        SPDLOG_INFO("Leader Is Ready for expected <index,term>=<{},{}>, appliedIndex={}",
                    lastIndex, expectedTerm, appliedIndex);
        return lastIndex;
      }
    }
  }
}

bool ReadonlyRaftCommandEventStore::isLeader() const {
  return mRaftImpl->getRaftRole() == raft::RaftRole::Leader;
}

void ReadonlyRaftCommandEventStore::loadEntriesThreadMain() {
  pthread_setname_np(pthread_self(), "CES_Load");
  while (mRunning) {
    uint64_t queueSize = 0;

    {
      /// READ LOCK
      std::shared_lock<std::shared_mutex> lock(mMutex);
      queueSize = mTaskQueue.size();
    }

    if (queueSize >= kMaxQueueSize) {
      usleep(1000);  /// 1ms
      continue;
    }

    uint64_t commitIndex = mRaftImpl->getCommitIndex();

    if (mLoadedIndex >= commitIndex) {
      usleep(1000);   /// 1ms
      continue;
    }

    TaskPtr taskPtr = std::make_shared<Task>();

    uint64_t ts1InNano = TimeUtil::currentTimeInNanos();
    uint64_t size = mRaftImpl->getEntries(mLoadedIndex + 1,
                                          commitIndex - mLoadedIndex, &taskPtr->entries);

    uint64_t ts2InNano = TimeUtil::currentTimeInNanos();
    SPDLOG_INFO("Load a Task, queueSize={}, entryNum={}, storageCost={}ms",
                queueSize, size, (ts2InNano - ts1InNano) / 1000000.0);

    mLoadedIndex += size;

    {
      /// WRITE LOCK
      std::unique_lock<std::shared_mutex> lock(mMutex);
      mTaskQueue.push_back(std::move(taskPtr));
    }
  }
}

void ReadonlyRaftCommandEventStore::decryptEntries(std::vector<raft::LogEntry> *entries,
                                                   std::list<CommandEvents> *bundles) {
  for (auto &entry : *entries) {
    if (entry.noop()) {
      continue;
    }

    if (mCrypto->isEnabled()) {
      if (entry.version().secret_key_version() == SecretKey::kInvalidSecKeyVersion) {
        /// for compatibility: if no version field, use oldest version
        const auto &allVersions = mCrypto->getDescendingVersions();
        assert(!allVersions.empty());
        auto oldestVersion = allVersions.back();
        assert(mCrypto->decrypt(entry.mutable_payload(), oldestVersion) == 0);
      } else {
        assert(mCrypto->decrypt(entry.mutable_payload(), entry.version().secret_key_version()) == 0);
      }
    }

    RaftPayload payload;
    assert(payload.ParseFromString(entry.payload()));

    if (payload.events_size() == 0) {
      SPDLOG_DEBUG("Command {} has no event", payload.command().id());
      continue;
    }

    std::unique_ptr<Command> command;
    std::list<std::unique_ptr<Event>> events;

    command = CommandEventDecodeWrapper::decodeCommand(payload.command(), *mCommandDecoder);
    for (const auto &event : payload.events()) {
      events.push_back(CommandEventDecodeWrapper::decodeEvent(event, *mEventDecoder));
    }

    bundles->push_back(std::make_pair(std::move(command), std::move(events)));
  }
}

void ReadonlyRaftCommandEventStore::decryptEntriesThreadMain() {
  pthread_setname_np(pthread_self(), "CES_Decrypt");

  while (mRunning) {
    TaskPtr taskPtr;

    {
      /// READ LOCK
      std::shared_lock<std::shared_mutex> lock(mMutex);

      for (auto &currTaskPtr : mTaskQueue) {
        uint64_t expected = 0;
        if (currTaskPtr->flag.compare_exchange_strong(expected, 1)) {
          taskPtr = currTaskPtr;
          break;
        }
      }
    }

    if (!taskPtr) {
      usleep(1000);   /// 1ms
      continue;
    }

    uint64_t ts1InNano = TimeUtil::currentTimeInNanos();
    decryptEntries(&taskPtr->entries, &taskPtr->bundles);

    uint64_t ts2InNano = TimeUtil::currentTimeInNanos();
    SPDLOG_INFO("Decrypt a Task, entryNum={}, decryptCost={}ms",
                taskPtr->entries.size(), (ts2InNano - ts1InNano) / 1000000.0);

    taskPtr->flag = 2;
  }
}

void ReadonlyRaftCommandEventStore::trySyncLoadBundles() {
  assert(mCachedBundles.empty());

  uint64_t commitIndex = mRaftImpl->getCommitIndex();
  ++mSpinTimes;

  /// mLoadedIndex is a persisted variable set by setCurrentOffset(),
  /// it may be legitimately greater than commitIndex.
  if (mLoadedIndex >= commitIndex) {
    if (mSpinTimes >= kSpinLimit) {
      usleep(1000);  /// 1ms
      mSpinTimes = 0;
    }
    return;
  }

  mSpinTimes = 0;

  std::list<CommandEvents> bundles;
  uint64_t size = loadBundles(mLoadedIndex + 1,
                              commitIndex - mLoadedIndex, &bundles);
  mLoadedIndex += size;
  mCachedBundles.swap(bundles);
}

void ReadonlyRaftCommandEventStore::tryAsyncLoadBundles() {
  assert(mCachedBundles.empty());

  uint64_t queueSize = 0;

  {
    /// READ LOCK
    std::shared_lock<std::shared_mutex> lock(mMutex);
    queueSize = mTaskQueue.size();
  }

  if (queueSize == 0) {
///    usleep(1000);  /// 1ms
    return;
  }

  TaskPtr taskPtr;

  {
    /// READ LOCK
    std::shared_lock<std::shared_mutex> lock(mMutex);
    taskPtr = mTaskQueue.front();
  }

  if (taskPtr->flag != 2) {
///    usleep(1000);  /// 1ms
    return;
  }

  {
    /// WRITE LOCK
    std::unique_lock<std::shared_mutex> lock(mMutex);
    mTaskQueue.pop_front();
  }

  mCachedBundles.swap(taskPtr->bundles);

  SPDLOG_INFO("Apply a Task, queueSize={}, entryNum={}",
              queueSize, taskPtr->entries.size());
}

}  /// namespace gringofts

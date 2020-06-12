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

#ifndef SRC_INFRA_RAFT_STORAGE_INMEMORYLOG_H_
#define SRC_INFRA_RAFT_STORAGE_INMEMORYLOG_H_

#include "Log.h"

namespace gringofts {
namespace storage {

/**
 * InMemory Storage.
 *
 * log layout: [mFirstIndex, mLastIndex]
 *             'mLastIndex = mFirstIndex - 1' indicate empty log
 */
class InMemoryLog : public Log {
 public:
  bool appendEntry(const raft::LogEntry &entry) override {
    if (mLastIndex + 1 != entry.index()) {
      return false;
    }

    ++mLastIndex;
    mLog.push_back(entry);
    return true;
  }

  bool appendEntries(const std::vector<raft::LogEntry> &entries) override {
    for (auto &entry : entries) {
      if (!appendEntry(entry)) {
        return false;
      }
    }
    return true;
  }

  bool getEntry(uint64_t index, raft::LogEntry *entry) const override {
    if (index == 0) {
      raft::LogEntry dummy;
      dummy.set_term(0);
      dummy.set_index(0);
      *entry = dummy;
      return true;
    }

    if (index < mFirstIndex || index > mLastIndex) {
      return false;
    }

    *entry = mLog[index - mFirstIndex];
    return true;
  }

  bool getTerm(uint64_t index, uint64_t *term) const override {
    if (index == 0) {
      *term = 0;
      return true;
    }

    if (index < mFirstIndex || index > mLastIndex) {
      return false;
    }

    *term = mLog[index - mFirstIndex].term();
    return true;
  }

  bool getEntries(uint64_t index, uint64_t size,
                  raft::LogEntry *entries) const override {
    if (index < mFirstIndex || index + size - 1 > mLastIndex) {
      return false;
    }

    for (uint64_t i = 0; i < size; ++i) {
      entries[i] = mLog[index + i - mFirstIndex];
    }
    return true;
  }

  uint64_t getEntries(const uint64_t startIndex,
                      const uint64_t maxLenInBytes, uint64_t maxBatchSize,
                      std::vector<raft::LogEntry> *entries) const override {
    uint64_t lastIndex = mLastIndex;

    /// heartbeat
    if (startIndex > lastIndex) {
      assert(startIndex == lastIndex + 1);
      return 0;
    }

    /// adjust maxBatchSize
    maxBatchSize = std::min(maxBatchSize, lastIndex - startIndex + 1);

    /// prepare entries
    entries->clear();
    entries->reserve(maxBatchSize);

    uint64_t batchSize = 0;
    uint64_t lenInBytes = 0;

    while (true) {
      if (batchSize >= maxBatchSize) {
        break;
      }

      uint64_t nextIndex = startIndex + batchSize;
      auto nextEntry = mLog[nextIndex - mFirstIndex];

      uint64_t nextLogSize = nextEntry.ByteSizeLong();

      if (lenInBytes + nextLogSize > maxLenInBytes) {
        break;
      }

      ++batchSize;
      lenInBytes += nextLogSize;
      entries->push_back(std::move(nextEntry));
    }

    SPDLOG_INFO("batchSize={}, lenInBytes={}", batchSize, lenInBytes);
    return batchSize;
  }

  void truncatePrefix(uint64_t firstIndexKept) override {
    if (mFirstIndex >= firstIndexKept) {
      SPDLOG_WARN("Nothing is going to happen, since "
                  "mFirstIndex={} >= firstIndexKept={}", mFirstIndex, firstIndexKept);
      return;
    }

    uint64_t prevFirstIndex = mFirstIndex;
    mFirstIndex = firstIndexKept;

    if (mFirstIndex > mLastIndex) {
      SPDLOG_WARN("Truncate all logs in InMemory log, "
                  "firstIndex={}, lastIndex={}", mFirstIndex, mLastIndex);
      mLastIndex = mFirstIndex - 1;
      mLog.clear();
    } else {
      SPDLOG_INFO("Truncate prefix, "
                  "firstIndex={}, lastIndex={}", mFirstIndex, mLastIndex);
      mLog.erase(mLog.begin(), mLog.begin() + mFirstIndex - prevFirstIndex);
    }
  }

  void truncateSuffix(uint64_t lastIndexKept) override {
    if (mLastIndex <= lastIndexKept) {
      SPDLOG_WARN("Nothing is going to happen, since "
                  "mLastIndex={} <= lastIndexKept={}", mLastIndex, lastIndexKept);
      return;
    }

    mLastIndex = lastIndexKept;

    if (mLastIndex < mFirstIndex) {
      SPDLOG_WARN("Truncate all logs in InMemory log, "
                  "firstIndex={}, lastIndex={}", mFirstIndex, mLastIndex);
      mFirstIndex = mLastIndex + 1;
      mLog.clear();
    } else {
      SPDLOG_INFO("Truncate suffix, "
                  "firstIndex={}, lastIndex={}", mFirstIndex, mLastIndex);
      mLog.resize(mLastIndex - mFirstIndex + 1);
    }
  }

  void setCurrentTerm(uint64_t term) override { mTerm = term; }
  void setVoteFor(uint64_t voteFor) override { mVoteFor = voteFor; }

  uint64_t getCurrentTerm() const override { return mTerm; }
  uint64_t getVote() const override { return mVoteFor; }

  uint64_t getFirstLogIndex() const override { return mFirstIndex; }
  uint64_t getLastLogIndex() const override { return mLastIndex; }

  std::string getName() const override { return "Memory"; }

 private:
  std::vector<raft::LogEntry> mLog;

  std::atomic<uint64_t> mFirstIndex = 1;
  std::atomic<uint64_t> mLastIndex = 0;

  std::atomic<uint64_t> mTerm = 0;
  std::atomic<uint64_t> mVoteFor = 0;
};

}  /// namespace storage
}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_STORAGE_INMEMORYLOG_H_

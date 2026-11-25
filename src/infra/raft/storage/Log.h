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

#ifndef SRC_INFRA_RAFT_STORAGE_LOG_H_
#define SRC_INFRA_RAFT_STORAGE_LOG_H_

#include <assert.h>
#include <cinttypes>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "../../util/CryptoUtil.h"
#include "../generated/raft.grpc.pb.h"

namespace gringofts {
namespace storage {

struct SerializedLogEntry {
  /// Aliasing ownership keeps either the mapped Segment or a private byte copy alive.
  std::shared_ptr<const char> mData;
  std::size_t mSize = 0;
};

/**
 * The base class for Raft Log Storage.
 */
class Log {
 public:
  Log() = default;
  virtual ~Log() = default;

  /// Log is not copyable
  Log(const Log &) = delete;
  Log &operator=(const Log &) = delete;

  virtual SecKeyVersion getLatestSecKeyVersion() const {
    return SecretKey::kInvalidSecKeyVersion;
  }

  /// append
  virtual bool appendEntry(const raft::LogEntry &entry) = 0;
  virtual bool appendEntries(const std::vector<raft::LogEntry> &entries) = 0;

  /// References are borrowed only for this synchronous append.
  virtual bool appendEntriesByRef(const std::vector<const raft::LogEntry *> &entries) {
    std::vector<raft::LogEntry> copies;
    copies.reserve(entries.size());
    for (const auto *entry : entries) {
      copies.push_back(*entry);
    }
    return appendEntries(copies);
  }

  /// kinds of get
  virtual bool getEntry(uint64_t index, raft::LogEntry *entry) const = 0;
  virtual bool getTerm(uint64_t index, uint64_t *term) const = 0;

  /// result will be filled into entries,
  /// which is an array with at least 'size' items.
  virtual bool getEntries(uint64_t index, uint64_t size,
                          raft::LogEntry *entries) const = 0;

  /// return actually fetched entry num,
  /// the fetched log entry is saved in entries.
  virtual uint64_t getEntries(const uint64_t startIndex,
                              const uint64_t maxLenInBytes, uint64_t maxBatchSize,
                              std::vector<raft::LogEntry> *entries) const = 0;

  /// Called by the log writer thread; returned bytes remain immutable until their owners are released.
  virtual uint64_t getSerializedEntries(uint64_t startIndex, uint64_t maxLenInBytes, uint64_t maxBatchSize,
                                       std::vector<SerializedLogEntry> *entries) const {
    entries->clear();
    std::vector<raft::LogEntry> decoded;
    auto count = getEntries(startIndex, maxLenInBytes, maxBatchSize, &decoded);
    if (count != decoded.size()) {
      return 0;
    }
    entries->reserve(count);
    for (const auto &entry : decoded) {
      auto bytes = std::make_shared<std::string>();
      if (!entry.SerializeToString(bytes.get())) {
        entries->clear();
        return 0;
      }
      entries->push_back({std::shared_ptr<const char>(bytes, bytes->data()), bytes->size()});
    }
    return count;
  }

  /// truncate prefix from [firstIndex, lastIndex] to [firstIndexKept, lastIndex]
  virtual void truncatePrefix(uint64_t firstIndexKept) = 0;

  /// truncate suffix from [firstIndex, lastIndex] to [firstIndex, lastIndexKept]
  virtual void truncateSuffix(uint64_t lastIndexKept) = 0;

  /// return firstIndex, initial value is 1
  virtual uint64_t getFirstLogIndex() const = 0;

  /// return lastIndex, initial value is 0
  virtual uint64_t getLastLogIndex() const = 0;

  /// recover/persist currentTerm and voteFor from/to storage.
  virtual void setCurrentTerm(uint64_t term) = 0;
  virtual void setVoteFor(uint64_t voteFor) = 0;

  virtual uint64_t getCurrentTerm() const = 0;
  virtual uint64_t getVote() const = 0;

  /// return name of log implementation, such as 'Memory' or 'SegmentLog'
  virtual std::string getName() const = 0;

 protected:
  /// log layout: [mFirstIndex, mLastIndex]
  ///             'mLastIndex = mFirstIndex - 1' indicate empty log
  static bool isEmpty(uint64_t firstIndex, uint64_t lastIndex) {
    return lastIndex == firstIndex - 1;
  }

 public:
  /**
   * Interface that will be deprecated later
   */
  void append(const raft::LogEntry &entry) {
    assert(appendEntry(entry));
  }
  void append(const std::vector<raft::LogEntry> &entries) {
    assert(appendEntries(entries));
  }

  uint64_t getLogTerm(uint64_t index) const {
    uint64_t term;
    assert(getTerm(index, &term));
    return term;
  }
};

}  /// namespace storage
}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_STORAGE_LOG_H_

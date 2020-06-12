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

#ifndef SRC_INFRA_RAFT_STORAGE_SEGMENT_H_
#define SRC_INFRA_RAFT_STORAGE_SEGMENT_H_

#include <atomic>

#include "../../util/CryptoUtil.h"
#include "../generated/raft.pb.h"

namespace gringofts {
namespace storage {

/**
 * There is no contention under multi Read/single Write on Segment.
 * It is wait-free to access Segment under multi-threaded environment.
 *
 * Leader:
 *
 *            StateMachine         |            AE_req | Client_req
 * --------------------------------|-------------------|
 *                                 |                   |
 *                             commitIndex           lastIndex
 *
 * 1) StateMachine will read entry before commitIndex.
 * 2) Leader will read entry before lastIndex, for preparing AE_req.
 * 3) Leader will append entry after lastIndex, for handling Client_req.
 *
 * entries before commitIndex is immutable and read only.
 * During leader's current term, log is append only, lastIndex increase monotonically.
 * Thus, memory location before lastIndex is read only, after lastIndex is write only
 *
 * Follower:
 *
 *            StateMachine         |   might truncate  | AE_req
 * --------------------------------|-------------------|
 *                                 |                   |
 *                             commitIndex           lastIndex
 *
 * 1) StateMachine will read entry before commitIndex.
 * 2) Follower might truncate entries between commitIndex and lastIndex,
 *    if AE consistency check failed.
 * 3) Follower will append entry after lastIndex, once it match leader.
 *
 * entries before commitIndex is immutable and read only.
 * memory location after commitIndex is write only, no one would read there.
 *
 */
class Segment final {
 public:
  /// Ctor for active segment
  /// need create
  Segment(const std::string &logDir, uint64_t firstIndex, uint64_t maxDataSize, uint64_t maxMetaSize,
          const std::shared_ptr<CryptoUtil> &crypto)
      : mDataSizeLimit(maxDataSize),
        mMetaSizeLimit(maxMetaSize),
        mLogDir(logDir + "/"),
        mIsActive(true),
        mFirstIndex(firstIndex),
        mLastIndex(firstIndex - 1),
        mCrypto(crypto) { createActiveSegment(); }

  /// Ctor for active segment
  /// need recover
  Segment(const std::string &logDir, uint64_t firstIndex,
          const std::shared_ptr<CryptoUtil> &crypto)
      : mLogDir(logDir + "/"),
        mIsActive(true),
        mFirstIndex(firstIndex),
        mLastIndex(firstIndex - 1),
        mCrypto(crypto) { /** lazy recover */ }

  /// Ctor for closed segment
  /// need recover
  Segment(const std::string &logDir, uint64_t firstIndex, uint64_t lastIndex,
          const std::shared_ptr<CryptoUtil> &crypto)
      : mLogDir(logDir + "/"),
        mIsActive(false),
        mFirstIndex(firstIndex),
        mLastIndex(lastIndex),
        mCrypto(crypto) { /** lazy recover */ }

  ~Segment();

  /// forbidden copy
  Segment(const Segment &) = delete;
  Segment &operator=(const Segment &) = delete;

  /// create or recover segment
  void createActiveSegment();
  void recoverActiveOrClosedSegment();

  /// convert active segment to closed segment
  /// just rename() from segment_in_progress_first to segment_first_last
  void closeActiveSegment();

  /// check if current Segment (must be active) has enough space to hold entries,
  /// return true if should roll
  bool shouldRoll(const std::vector<raft::LogEntry> &entries) const;

  /// require: call shouldRoll() ahead to make sure that
  ///          current Segment has enough space to hold entries.
  void appendEntries(const std::vector<raft::LogEntry> &entries);

  /// kinds of get method
  bool getEntry(uint64_t index, raft::LogEntry *entry) const;
  bool getTerm(uint64_t index, uint64_t *term) const;

  bool getEntries(uint64_t index, uint64_t size, raft::LogEntry *entries) const;
  uint64_t getEntries(const uint64_t startIndex,
                      const uint64_t maxLenInBytes, uint64_t maxBatchSize,
                      std::vector<raft::LogEntry> *entries) const;

  uint64_t getFirstIndex() const { return mFirstIndex; }
  uint64_t getLastIndex() const { return mLastIndex; }

  /// truncate from [firstIndex, lastIndex] to [firstIndex, lastIndexKept]
  /// Attention: rename() is needed for closed segment,
  ///            since its name is segment_first_last, and last changed.
  void truncateSuffix(uint64_t lastIndexKept);

  /// assume current file name is abc.data, abc.meta,
  /// rename to abc.data.dropped.tsInNano, abc.meta.dropped.tsInNano
  void dropSegment() const;

 private:
  struct LogMeta;

  /// check if index within [mFirstIndex, mLastIndex]
  bool isWithInBoundary(uint64_t index) const;

  /// require: index within [mFirstIndex, mLastIndex]
  const LogMeta &getMeta(uint64_t index) const;

  /// require: offset within [0, mDataOffset]
  void *getEntryAddr(uint64_t offset) const;

  /// log layout: [mFirstIndex, mLastIndex]
  ///             'mLastIndex = mFirstIndex - 1' indicate empty log
  static bool isEmpty(uint64_t firstIndex, uint64_t lastIndex) {
    return lastIndex == firstIndex - 1;
  }

  /// create HMAC based on index and payload
  /// Attention: all fields of LogMeta except digest should be ready.
  void createHMAC(LogMeta *meta) const;

  /// verify HMAC based on index and payload
  /// Attention: take effect iff this entry has HMAC and HMAC is enabled in crypto.
  void verifyHMAC(const LogMeta &meta) const;

  /// field for data
  uint64_t mDataSizeLimit;
  uint64_t mDataOffset = 0;
  void *mDataMemPtr = nullptr;
  int mDataFd = -1;

  /// field for meta
  uint64_t mMetaSizeLimit;
  uint64_t mMetaOffset = 0;
  void *mMetaMemPtr = nullptr;
  int mMetaFd = -1;

  /// index range
  uint64_t mFirstIndex;
  std::atomic<uint64_t> mLastIndex;

  std::string mLogDir;
  bool mIsActive;

  /// crypto for HMAC
  std::shared_ptr<CryptoUtil> mCrypto;
};

}  /// namespace storage
}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_STORAGE_SEGMENT_H_

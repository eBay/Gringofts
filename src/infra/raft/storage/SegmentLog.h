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

#ifndef SRC_INFRA_RAFT_STORAGE_SEGMENTLOG_H_
#define SRC_INFRA_RAFT_STORAGE_SEGMENTLOG_H_

#include <atomic>
#include <map>
#include <memory>
#include <mutex>

#include "../../util/CryptoUtil.h"
#include "../../util/FileUtil.h"
#include "Log.h"
#include "Segment.h"

namespace gringofts {
namespace storage {

/**
 * Log use segmented append-only file
 *
 * The threading model should be:
 *      single writer thread, call append or truncate
 *      multi reader threads, call kinds of get
 *
 * SegmentLog layout:
 *      current_term                         : record currentTerm of raft
 *      vote_for                             : record voteFor of raft
 *      first_index                          : record firstIndex of segment log
 *      segment_first_last.meta/data         : closed segment
 *      segment_in_progress_first.meta/data  : active segment
 *
 *      first        last   first        last   first    last
 *       |            |      |            |      |        |
 *       --------------      --------------      ----------
 *       |            | ---> |            | ---> |        |
 *       --------------      --------------      ----------
 *           |                                            |
 *          mFirstIndex                                  mLastIndex
 *
 */
class SegmentLog : public Log {
 public:
  SegmentLog(const std::string &logDir,
             const std::shared_ptr<CryptoUtil> &crypto,
             uint64_t segmentDataSizeLimit,
             uint64_t segmentMetaSizeLimit)
      : mLogDir(logDir),
        mMetaStorage(logDir),
        mCrypto(crypto),
        mSegmentDataSizeLimit(segmentDataSizeLimit),
        mSegmentMetaSizeLimit(segmentMetaSizeLimit) { init(); }

  ~SegmentLog() override = default;

  void init();

  /// append
  bool appendEntry(const raft::LogEntry &entry) override;
  bool appendEntries(const std::vector<raft::LogEntry> &entries) override;

  /// kinds of get
  bool getEntry(uint64_t index, raft::LogEntry *entry) const override;
  bool getTerm(uint64_t index, uint64_t *term) const override;

  bool getEntries(uint64_t index, uint64_t size,
                  raft::LogEntry *entries) const override { assert(0); /** does not support */ }

  uint64_t getEntries(const uint64_t startIndex,
                      const uint64_t maxLenInBytes, uint64_t maxBatchSize,
                      std::vector<raft::LogEntry> *entries) const override;

  /// truncate prefix/suffix
  void truncatePrefix(uint64_t firstIndexKept) override;
  void truncateSuffix(uint64_t lastIndexKept) override;

  /// get/set meta data
  void setCurrentTerm(uint64_t term) override {
    mMetaStorage.persistCurrentTerm(term);
    mCurrentTerm = term;
  }

  void setVoteFor(uint64_t voteFor) override {
    mMetaStorage.persistVoteFor(voteFor);
    mVoteFor = voteFor;
  }

  uint64_t getCurrentTerm() const override { return mCurrentTerm; }
  uint64_t getVote() const override { return mVoteFor; }

  uint64_t getFirstLogIndex() const override { return mFirstIndex; }
  uint64_t getLastLogIndex() const override { return mLastIndex; }

  std::string getName() const override { return "SegmentLog"; }

 private:
  /**
   * Meta Storage of Segment Log,
   * such as currentTerm, voteFor, firstIndex
   */
  struct MetaStorage {
    explicit MetaStorage(const std::string &logDir);

    /// persist to disk
    void persistCurrentTerm(uint64_t term) const;
    void persistVoteFor(uint64_t vote) const;
    void persistFirstIndex(uint64_t index) const;

    /// recover from disk
    uint64_t recoverCurrentTerm() const;
    uint64_t recoverVoteFor() const;
    uint64_t recoverFirstIndex() const;

    /**
     * ATTENTION: one file for one field:
     *            separate the storage for different field to avoid contention
     *            between simultaneously modification on different field.
     *
     *            In a long term way, If we want to modify multi fields
     *            in a single action (such as incr currentTerm and clear voteFor),
     *            We need support ACID for meta data modification
     */
    std::string currentTermFilePath;
    std::string voteForFilePath;
    std::string firstIndexFilePath;
  };

  /// mapping table for closed segments
  using SegmentPtr = std::shared_ptr<Segment>;
  using SegmentMap = std::map<uint64_t /**firstIndex*/, SegmentPtr>;

  /// recover closed/active segments
  void listSegments();
  void recoverSegments();

  /// close current active segment, roll out a new one.
  SegmentPtr rollActiveSegment();

  /// given index, return segment (active or closed) that holding it
  /// return empty shared_ptr if there is no such segment.
  SegmentPtr getSegment(uint64_t index) const;

  const std::string mLogDir;
  MetaStorage mMetaStorage;

  /// crypto for HMAC
  std::shared_ptr<CryptoUtil> mCrypto;

  const uint64_t mSegmentDataSizeLimit;
  const uint64_t mSegmentMetaSizeLimit;

  std::atomic<uint64_t> mFirstIndex = 1;
  std::atomic<uint64_t> mLastIndex = 0;

  std::atomic<uint64_t> mCurrentTerm = 0;
  std::atomic<uint64_t> mVoteFor = 0;

  /**
   * Must lock mMutex before access or modify mClosedSegments or mActiveSegment
   * After got a SegmentPtr, operations on that SegmentPtr is thread-safe.
   */
  mutable std::mutex mMutex;
  SegmentMap mClosedSegments;
  std::shared_ptr<Segment> mActiveSegment;
};

}  /// namespace storage
}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_STORAGE_SEGMENTLOG_H_

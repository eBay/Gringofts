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

#include "SegmentLog.h"

#include <regex>

#include "../../util/TimeUtil.h"

namespace gringofts {
namespace storage {

SegmentLog::MetaStorage::MetaStorage(const std::string &logDir)
    : currentTermFilePath(logDir + "/current_term"),
      voteForFilePath(logDir + "/vote_for"),
      firstIndexFilePath(logDir + "/first_index") {}

void SegmentLog::MetaStorage::persistCurrentTerm(uint64_t term) const {
  FileUtil::setFileContentWithSync(currentTermFilePath, std::to_string(term));
}

void SegmentLog::MetaStorage::persistVoteFor(uint64_t vote) const {
  FileUtil::setFileContentWithSync(voteForFilePath, std::to_string(vote));
}

void SegmentLog::MetaStorage::persistFirstIndex(uint64_t index) const {
  FileUtil::setFileContentWithSync(firstIndexFilePath, std::to_string(index));
}

uint64_t SegmentLog::MetaStorage::recoverCurrentTerm() const {
  if (FileUtil::fileExists(currentTermFilePath)) {
    return std::stoull(FileUtil::getFileContent(currentTermFilePath));
  }
  /// initial value of currentTerm is 0
  persistCurrentTerm(0);
  return 0;
}

uint64_t SegmentLog::MetaStorage::recoverVoteFor() const {
  if (FileUtil::fileExists(voteForFilePath)) {
    return std::stoull(FileUtil::getFileContent(voteForFilePath));
  }
  /// initial value of voteFor is 0
  persistVoteFor(0);
  return 0;
}

uint64_t SegmentLog::MetaStorage::recoverFirstIndex() const {
  if (FileUtil::fileExists(firstIndexFilePath)) {
    return std::stoull(FileUtil::getFileContent(firstIndexFilePath));
  }
  /// initial value of firstIndex is 1
  persistFirstIndex(1);
  return 1;
}

void SegmentLog::init() {
  /// recover meta data
  mCurrentTerm = mMetaStorage.recoverCurrentTerm();
  mVoteFor = mMetaStorage.recoverVoteFor();
  mFirstIndex = mMetaStorage.recoverFirstIndex();

  /// recover closed/active segments
  listSegments();
  recoverSegments();

  /// handle empty storage
  if (mLastIndex == 0) {
    mLastIndex = mFirstIndex - 1;
  }
  /// make sure there is an active segment
  if (!mActiveSegment) {
    mActiveSegment = std::make_shared<Segment>(mLogDir, mLastIndex + 1,
                                               mSegmentDataSizeLimit, mSegmentMetaSizeLimit, mCrypto);
  }
}

void SegmentLog::listSegments() {
  /// regex pattern for closedSegment
  std::regex closedSegmentRegex("segment_([0-9]+)_([0-9]+).meta$");
  std::smatch closedSegment;

  /// regex pattern for activeSegment
  std::regex activeSegmentRegex("segment_in_progress_([0-9]+).meta$");
  std::smatch activeSegment;

  auto fileNames = FileUtil::listFiles(mLogDir);

  for (const auto &fileName : fileNames) {
    if (std::regex_search(fileName, closedSegment, closedSegmentRegex)) {
      uint64_t firstIndex = std::stoull(closedSegment[1]);
      uint64_t lastIndex = std::stoull(closedSegment[2]);
      SPDLOG_INFO("find closed segment: {}, firstIndex={}, lastIndex={}",
                  fileName, firstIndex, lastIndex);

      assert(mClosedSegments.find(firstIndex) == mClosedSegments.end());
      mClosedSegments[firstIndex] = std::make_shared<Segment>(
          mLogDir, firstIndex, lastIndex, mCrypto);
    } else if (std::regex_search(fileName, activeSegment, activeSegmentRegex)) {
      uint64_t firstIndex = std::stoull(activeSegment[1]);
      SPDLOG_INFO("find active segment: {}, firstIndex={}",
                  fileName, firstIndex);
      assert(!mActiveSegment);
      mActiveSegment = std::make_shared<Segment>(mLogDir, firstIndex, mCrypto);
    } else {
      SPDLOG_INFO("ignore file: {}", fileName);
    }
  }
}

void SegmentLog::recoverSegments() {
  SegmentPtr prevSegPtr;

  /// verify closedSegment
  for (auto iter = mClosedSegments.begin(); iter != mClosedSegments.end();) {
    auto segmentPtr = iter->second;

    assert(segmentPtr->getFirstIndex() <= segmentPtr->getLastIndex());
    assert(!prevSegPtr || segmentPtr->getFirstIndex() == prevSegPtr->getLastIndex() + 1);
    assert(prevSegPtr || mFirstIndex >= segmentPtr->getFirstIndex());

    /// ignore retained segment
    if (!prevSegPtr && mFirstIndex > segmentPtr->getLastIndex()) {
      SPDLOG_INFO("ignore retained closed segment");
      iter = mClosedSegments.erase(iter);
      continue;
    }

    /// recover closed segment
    SPDLOG_INFO("load closed segment");
    segmentPtr->recoverActiveOrClosedSegment();

    mLastIndex = segmentPtr->getLastIndex();
    prevSegPtr = segmentPtr;
    ++iter;
  }

  /// verify activeSegment
  if (mActiveSegment) {
    assert(prevSegPtr || mFirstIndex >= mActiveSegment->getFirstIndex());
    assert(!prevSegPtr || mActiveSegment->getFirstIndex() == prevSegPtr->getLastIndex() + 1);

    mActiveSegment->recoverActiveOrClosedSegment();

    /// ATTENTION: lastIndex of active segment
    ///            is recovered by Segment::recoverActiveOrClosedSegment()

    /// ignore retained segment
    if (mFirstIndex > mActiveSegment->getLastIndex()) {
      SPDLOG_INFO("ignore retained active segment");
      mActiveSegment.reset();
    } else {
      mLastIndex = mActiveSegment->getLastIndex();
    }
  }
}

SegmentLog::SegmentPtr SegmentLog::rollActiveSegment() {
  mActiveSegment->closeActiveSegment();

  std::lock_guard<std::mutex> lock(mMutex);

  mClosedSegments[mActiveSegment->getFirstIndex()] = std::move(mActiveSegment);
  mActiveSegment = std::make_shared<Segment>(mLogDir, mLastIndex + 1,
                                             mSegmentDataSizeLimit, mSegmentMetaSizeLimit, mCrypto);
  return mActiveSegment;
}

SegmentLog::SegmentPtr SegmentLog::getSegment(uint64_t index) const {
  std::lock_guard<std::mutex> lock(mMutex);

  uint64_t firstIndex = mFirstIndex;
  uint64_t lastIndex = mLastIndex;

  if (isEmpty(firstIndex, lastIndex)) {
    SPDLOG_WARN("Empty Storage, [{}, {}]", firstIndex, lastIndex);
    return nullptr;
  }

  if (index < firstIndex || index > lastIndex) {
    SPDLOG_WARN("Access log entry {}, which is out of boundary [{}, {}]",
                index, firstIndex, lastIndex);
    return nullptr;
  }

  if (mActiveSegment && index >= mActiveSegment->getFirstIndex()) {
    return mActiveSegment;
  }

  assert(!mClosedSegments.empty());
  auto iter = mClosedSegments.upper_bound(index);
  return (--iter)->second;
}

bool SegmentLog::getEntry(uint64_t index, raft::LogEntry *entry) const {
  auto segmentPtr = getSegment(index);

  /// segment corresponding to index is removed by truncatedPrefix
  if (!segmentPtr) {
    return false;
  }

  return segmentPtr->getEntry(index, entry);
}

bool SegmentLog::getTerm(uint64_t index, uint64_t *term) const {
  /// if a follower with empty log starts at a specified firstIndex
  /// that equals to leader's firstIndex, the <prevLogIndex, preLogTerm>
  /// for AE consistency check will trigger this case.
  if (index == mFirstIndex - 1) {
    *term = 0;
    return true;
  }

  auto segmentPtr = getSegment(index);
  return segmentPtr ? segmentPtr->getTerm(index, term) : false;
}

uint64_t SegmentLog::getEntries(const uint64_t startIndex,
                                const uint64_t maxLenInBytes, uint64_t maxBatchSize,
                                std::vector<raft::LogEntry> *entries) const {
  uint64_t lastIndex = mLastIndex;

  /// heartbeat
  if (startIndex > lastIndex) {
    assert(startIndex == lastIndex + 1);
    return 0;
  }

  auto segmentPtr = getSegment(startIndex);

  /// segment corresponding to startIndex is removed by truncatedPrefix
  if (!segmentPtr) {
    return 0;
  }

  return segmentPtr->getEntries(startIndex, maxLenInBytes, maxBatchSize, entries);
}

bool SegmentLog::appendEntry(const raft::LogEntry &entry) {
  if (entry.index() != mLastIndex + 1) {
    return false;
  }

  SegmentPtr activeSegment;

  {
    std::lock_guard<std::mutex> lock(mMutex);
    activeSegment = mActiveSegment;
  }

  assert(activeSegment);

  /// roll if needed
  if (activeSegment->shouldRoll({entry})) {
    activeSegment = rollActiveSegment();
  }

  activeSegment->appendEntries({entry});
  ++mLastIndex;
  return true;
}

bool SegmentLog::appendEntries(const std::vector<raft::LogEntry> &entries) {
  if (entries.empty()) {
    return true;
  }
  if (entries[0].index() != mLastIndex + 1) {
    return false;
  }

  SegmentPtr activeSegment;

  {
    std::lock_guard<std::mutex> lock(mMutex);
    activeSegment = mActiveSegment;
  }

  assert(activeSegment);

  /// roll if needed
  if (activeSegment->shouldRoll(entries)) {
    activeSegment = rollActiveSegment();
  }

  activeSegment->appendEntries(entries);
  mLastIndex += entries.size();
  return true;
}

void SegmentLog::truncatePrefix(uint64_t firstIndexKept) {
  if (mFirstIndex >= firstIndexKept) {
    SPDLOG_WARN("Nothing is going to happen, since "
                "mFirstIndex={} >= firstIndexKept={}", mFirstIndex, firstIndexKept);
    return;
  }

  /// NOTE: truncatePrefix is not important, as it has nothing to do with
  ///       consensus. We try to save meta on the disk first to make sure
  ///       even if the process crashes (which is unlikely to happen),
  ///       the new process would see the latest 'first_log_index'
  mMetaStorage.persistFirstIndex(firstIndexKept);
  mFirstIndex = firstIndexKept;

  std::lock_guard<std::mutex> lock(mMutex);

  /// closed segments
  for (auto iter = mClosedSegments.begin(); iter != mClosedSegments.end();) {
    auto segmentPtr = iter->second;
    if (segmentPtr->getLastIndex() < firstIndexKept) {
      iter = mClosedSegments.erase(iter);
    } else {
      return;
    }
  }

  /// active segment
  assert(mActiveSegment);
  if (mActiveSegment->getLastIndex() < firstIndexKept) {
    /// reset empty Storage
    mLastIndex = mFirstIndex - 1;
    mActiveSegment = std::make_shared<Segment>(mLogDir, mLastIndex + 1,
                                               mSegmentDataSizeLimit, mSegmentMetaSizeLimit, mCrypto);
  }
}

void SegmentLog::truncateSuffix(uint64_t lastIndexKept) {
  if (mLastIndex <= lastIndexKept) {
    SPDLOG_WARN("Nothing is going to happen, since "
                "mLastIndex={} <= lastIndexKept={}", mLastIndex, lastIndexKept);
    return;
  }
  mLastIndex = lastIndexKept;

  SegmentPtr lastSegment;
  std::vector<SegmentPtr> dropped;

  do {
    std::lock_guard<std::mutex> lock(mMutex);

    /// active segment
    assert(mActiveSegment);
    if (mActiveSegment->getFirstIndex() <= lastIndexKept) {
      lastSegment = mActiveSegment;
      break;
    }

    /// drop mActiveSegment
    dropped.push_back(std::move(mActiveSegment));

    /// closed segments
    for (auto iter = mClosedSegments.rbegin(); iter != mClosedSegments.rend(); ++iter) {
      if (iter->second->getFirstIndex() <= lastIndexKept) {
        lastSegment = iter->second;
        break;
      }
      dropped.push_back(iter->second);
    }

    /// erase() can not work on reverse_iterator, erase by key here.
    for (const auto &segmentPtr : dropped) {
      mClosedSegments.erase(segmentPtr->getFirstIndex());
    }

    /// if all the logs have been cleared, adjust mFirstIndex
    if (mClosedSegments.empty()) {
      mFirstIndex = mLastIndex + 1;
    }
  } while (0);

  /// handle lastSegment
  if (lastSegment) {
    if (mFirstIndex <= mLastIndex) {
      lastSegment->truncateSuffix(lastIndexKept);
    } else {
      /// truncate_prefix() and truncate_suffix() to discard entire logs
      /// we need drop lastSegment
      dropped.push_back(lastSegment);

      std::lock_guard<std::mutex> lock(mMutex);
      if (lastSegment == mActiveSegment) {
        mActiveSegment.reset();
      } else {
        mClosedSegments.erase(lastSegment->getFirstIndex());
      }
    }
  }

  /// handle dropped
  for (auto &segmentPtr : dropped) {
    segmentPtr->dropSegment();
  }

  {
    std::lock_guard<std::mutex> lock(mMutex);

    /// make sure there is an active segment
    if (!mActiveSegment) {
      mActiveSegment = std::make_shared<Segment>(mLogDir, mLastIndex + 1,
                                                 mSegmentDataSizeLimit, mSegmentMetaSizeLimit, mCrypto);
    }
  }
}

}  /// namespace storage
}  /// namespace gringofts

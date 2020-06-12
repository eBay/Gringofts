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

#include "Segment.h"

#include <vector>

#include <spdlog/spdlog.h>

#include "../../util/FileUtil.h"
#include "../../util/TimeUtil.h"

namespace {

/**
 * fileName of data/meta for active/closed segment
 */
std::string dataFileNameForActiveSegment(uint64_t firstIndex) {
  return "segment_in_progress_" + std::to_string(firstIndex) + ".data";
}
std::string metaFileNameForActiveSegment(uint64_t firstIndex) {
  return "segment_in_progress_" + std::to_string(firstIndex) + ".meta";
}

std::string dataFileNameForClosedSegment(uint64_t firstIndex, uint64_t lastIndex) {
  return "segment_" + std::to_string(firstIndex) + "_" + std::to_string(lastIndex) + ".data";
}

std::string metaFileNameForClosedSegment(uint64_t firstIndex, uint64_t lastIndex) {
  return "segment_" + std::to_string(firstIndex) + "_" + std::to_string(lastIndex) + ".meta";
}

}  /// namespace

namespace gringofts {
namespace storage {

struct Segment::LogMeta {
  uint64_t offset = 0;
  uint64_t length = 0;
  uint64_t term = 0;
  uint64_t index = 0;
  char digest[32];      /// HMAC_SHA256
};

Segment::~Segment() {
  if (mDataFd == -1 && mMetaFd == -1) {
    /// not created or recovered
    return;
  }

  auto beg = TimeUtil::currentTimeInNanos();

  assert(::munmap(mDataMemPtr, mDataSizeLimit) == 0);
  assert(::close(mDataFd) == 0);

  assert(::munmap(mMetaMemPtr, mMetaSizeLimit) == 0);
  assert(::close(mMetaFd) == 0);

  auto end = TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("Destruct Segment Object, firstIndex={}, lastIndex={}, "
              "dataSize={}MB, metaSize={}MB, timeCost={}ms",
              mFirstIndex, mLastIndex,
              mDataOffset / 1024.0 / 1024.0, mMetaOffset / 1024.0 / 1024.0,
              (end - beg) / 1000000.0);
}

void Segment::createActiveSegment() {
  assert(mIsActive);

  auto beg = TimeUtil::currentTimeInNanos();

  /// open data file
  auto dataPath = mLogDir + dataFileNameForActiveSegment(mFirstIndex);
  mDataFd = ::open(dataPath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
  assert(mDataFd != -1);

  /// set file size, mmap
  FileUtil::setFileSize(mDataFd, mDataSizeLimit);
  mDataMemPtr = ::mmap(nullptr, mDataSizeLimit, PROT_WRITE | PROT_READ, MAP_SHARED, mDataFd, 0);
  assert(mDataMemPtr != MAP_FAILED);

  /// open meta file
  auto metaPath = mLogDir + metaFileNameForActiveSegment(mFirstIndex);
  mMetaFd = ::open(metaPath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
  assert(mMetaFd != -1);

  /// set file size, mmap
  FileUtil::setFileSize(mMetaFd, mMetaSizeLimit);
  mMetaMemPtr = ::mmap(nullptr, mMetaSizeLimit, PROT_WRITE | PROT_READ, MAP_SHARED, mMetaFd, 0);
  assert(mMetaMemPtr != MAP_FAILED);

  auto end = TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("Create an activeSegment, timeCost={}ms", (end - beg) / 1000000.0);
}

void Segment::recoverActiveOrClosedSegment() {
  auto beg = TimeUtil::currentTimeInNanos();

  /// open data file
  auto dataPath = mLogDir + (mIsActive ? dataFileNameForActiveSegment(mFirstIndex)
                                       : dataFileNameForClosedSegment(mFirstIndex, mLastIndex));

  mDataFd = ::open(dataPath.c_str(), O_RDWR);
  assert(mDataFd != -1);

  /// get file size, mmap
  mDataSizeLimit = FileUtil::getFileSize(mDataFd);
  mDataMemPtr = ::mmap(nullptr, mDataSizeLimit, PROT_WRITE | PROT_READ, MAP_SHARED, mDataFd, 0);
  assert(mDataMemPtr != MAP_FAILED);

  /// open meta file
  auto metaPath = mLogDir + (mIsActive ? metaFileNameForActiveSegment(mFirstIndex)
                                       : metaFileNameForClosedSegment(mFirstIndex, mLastIndex));

  mMetaFd = ::open(metaPath.c_str(), O_RDWR);
  assert(mMetaFd != -1);

  /// get file size, mmap
  mMetaSizeLimit = FileUtil::getFileSize(mMetaFd);
  mMetaMemPtr = ::mmap(nullptr, mMetaSizeLimit, PROT_WRITE | PROT_READ, MAP_SHARED, mMetaFd, 0);
  assert(mMetaMemPtr != MAP_FAILED);

  /// recover meta file
  uint64_t maxPos = mMetaSizeLimit / sizeof(LogMeta);
  const LogMeta *metaPtr = reinterpret_cast<LogMeta *>(mMetaMemPtr);

  uint64_t pos = 0;
  uint64_t lastIndex = mFirstIndex - 1;

  while (pos < maxPos && metaPtr[pos].index == lastIndex + 1) {
    ++pos;
    ++lastIndex;
  }

  /// corruption is not tolerate for a closed segment
  assert(mIsActive || lastIndex == mLastIndex);

  /// recover mLastIndex
  mLastIndex = lastIndex;

  /// recover mMetaOffset
  mMetaOffset = (mLastIndex - mFirstIndex + 1) * sizeof(LogMeta);
  assert(mMetaOffset <= mMetaSizeLimit);

  /// recover mDataOffset
  mDataOffset = isEmpty(mFirstIndex, mLastIndex)
                ? 0 : metaPtr[mLastIndex - mFirstIndex].offset + metaPtr[mLastIndex - mFirstIndex].length;
  assert(mDataOffset <= mDataSizeLimit);

  auto end = TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("Recover segment, maxDataSize={}MB, maxMetaSize={}MB, "
              "firstIndex={}, lastIndex={}, timeCost={}ms",
              mDataSizeLimit / 1024.0 / 1024.0, mMetaSizeLimit / 1024.0 / 1024.0,
              mFirstIndex, mLastIndex, (end - beg) / 1000000.0);
}

void Segment::closeActiveSegment() {
  assert(mIsActive);

  auto dataFromPath = mLogDir + dataFileNameForActiveSegment(mFirstIndex);
  auto dataToPath = mLogDir + dataFileNameForClosedSegment(mFirstIndex, mLastIndex);

  /// TODO(bigeng): how to recover if rename() failed ?
  ///               meta fileName will mismatch with data fileName
  assert(::rename(dataFromPath.c_str(), dataToPath.c_str()) == 0);

  auto metaFromPath = mLogDir + metaFileNameForActiveSegment(mFirstIndex);
  auto metaToPath = mLogDir + metaFileNameForClosedSegment(mFirstIndex, mLastIndex);

  assert(::rename(metaFromPath.c_str(), metaToPath.c_str()) == 0);

  mIsActive = false;
  SPDLOG_INFO("Close an activeSegment, "
              "firstIndex={}, lastIndex={}, dataSize={}MB, metaSize={}MB",
              mFirstIndex, mLastIndex,
              mDataOffset / 1024.0 / 1024.0, mMetaOffset / 1024.0 / 1024.0);
}

bool Segment::shouldRoll(const std::vector<raft::LogEntry> &entries) const {
  assert(mIsActive);

  uint64_t metaLen = entries.size() * sizeof(LogMeta);

  if (mMetaOffset + metaLen > mMetaSizeLimit) {
    SPDLOG_INFO("Rolling a new Segment due to MetaFile exceed, "
                "metaOffset={}, metaLenRequired={}, metaSizeLimit={}.",
                mMetaOffset, metaLen, mMetaSizeLimit);
    return true;
  }

  uint64_t dataLen = 0;

  for (auto &entry : entries) {
    dataLen += entry.ByteSizeLong();
  }

  if (mDataOffset + dataLen > mDataSizeLimit) {
    SPDLOG_INFO("Rolling a new Segment due to DataFile exceed, "
                "dataOffset={}, dataLenRequired={}, dataSizeLimit={}",
                mDataOffset, dataLen, mDataSizeLimit);
    return true;
  }

  return false;
}

void Segment::appendEntries(const std::vector<raft::LogEntry> &entries) {
  assert(mIsActive);

  auto beg = TimeUtil::currentTimeInNanos();

  std::vector<LogMeta> metaArr;
  metaArr.resize(entries.size());

  auto dataOffset = mDataOffset;

  /// copy to data file
  for (std::size_t i = 0; i < entries.size(); ++i) {
    auto &meta = metaArr[i];
    auto &entry = entries[i];

    assert(mLastIndex + 1 + i == entry.index());

    /// make sure LogMeta.digest is all zero
    ::memset(&meta, 0, sizeof(LogMeta));

    meta.index = entry.index();
    meta.term = entry.term();

    meta.offset = dataOffset;
    meta.length = entry.ByteSizeLong();

    /// Serialize to mmap memory
    void *entryAddr = reinterpret_cast<uint8_t *>(mDataMemPtr) + meta.offset;
    entry.SerializeToArray(entryAddr, meta.length);

    /// make sure about data safety
    createHMAC(&meta);

    /// update dataOffset
    dataOffset += meta.length;
  }

  /// sync data file
  void *dataAddr = reinterpret_cast<uint8_t *>(mDataMemPtr) + metaArr[0].offset;
  uint64_t dataLen = dataOffset - mDataOffset;
  FileUtil::syncAt(dataAddr, dataLen);

  /// copy to meta file and sync
  void *metaAddr = reinterpret_cast<uint8_t *>(mMetaMemPtr) + mMetaOffset;
  uint64_t metaLen = entries.size() * sizeof(LogMeta);

  ::memmove(metaAddr, metaArr.data(), metaLen);
  FileUtil::syncAt(metaAddr, metaLen);

  /// update mDataOffset and mMetaOffset
  mDataOffset = dataOffset;
  mMetaOffset += metaLen;

  /// update atomic var mLastIndex,
  /// so that latest change could be seen by other reading thread.
  mLastIndex += entries.size();

  auto end = TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("Append {} entry, lastIndex={}, dataLen={}KB, metaLen={}KB, timeCost={}ms",
              entries.size(), mLastIndex, dataLen / 1024.0, metaLen / 1024.0, (end - beg) / 1000000.0);
}

bool Segment::isWithInBoundary(uint64_t index) const {
  if (isEmpty(mFirstIndex, mLastIndex)) {
    SPDLOG_WARN("Segment is empty, firstIndex={}, lastIndex={}, require {}",
                mFirstIndex, mLastIndex, index);
    return false;
  }
  if (index < mFirstIndex || index > mLastIndex) {
    SPDLOG_WARN("Out of bound, firstIndex={}, lastIndex={}, require {}",
                mFirstIndex, mLastIndex, index);
    return false;
  }

  return true;
}

const Segment::LogMeta &Segment::getMeta(uint64_t index) const {
  assert(isWithInBoundary(index));
  const LogMeta *metaArr = reinterpret_cast<LogMeta *>(mMetaMemPtr);

  const auto &meta = metaArr[index - mFirstIndex];
  assert(meta.index == index);

  return meta;
}

void *Segment::getEntryAddr(uint64_t offset) const {
  assert(offset <= mDataOffset);
  return reinterpret_cast<uint8_t *>(mDataMemPtr) + offset;
}

bool Segment::getEntry(uint64_t index, raft::LogEntry *entry) const {
  if (!isWithInBoundary(index)) {
    return false;
  }

  auto &meta = getMeta(index);

  /// make sure about data safety
  verifyHMAC(meta);

  void *entryAddr = getEntryAddr(meta.offset);
  return entry->ParseFromArray(entryAddr, meta.length);
}

bool Segment::getTerm(uint64_t index, uint64_t *term) const {
  if (!isWithInBoundary(index)) {
    return false;
  }

  auto &meta = getMeta(index);
  *term = meta.term;
  return true;
}

bool Segment::getEntries(uint64_t index, uint64_t size, raft::LogEntry *entries) const {
  if (!isWithInBoundary(index) || !isWithInBoundary(index + size - 1)) {
    return false;
  }

  auto beg = TimeUtil::currentTimeInNanos();

  for (std::size_t i = 0; i < size; ++i) {
    auto &meta = getMeta(index + i);

    /// make sure about data safety
    verifyHMAC(meta);

    void *entryAddr = getEntryAddr(meta.offset);

    auto ret = entries[i].ParseFromArray(entryAddr, meta.length);
    if (!ret) {
      SPDLOG_WARN("failed to serialize LogEntry at {}", index + i);
      return false;
    }
  }

  auto end = TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("getEntries [{}, {}], timeCost={}ms",
              index, index + size - 1, (end - beg) / 1000000.0);
  return true;
}

uint64_t Segment::getEntries(const uint64_t startIndex,
                             const uint64_t maxLenInBytes, uint64_t maxBatchSize,
                             std::vector<raft::LogEntry> *entries) const {
  assert(isWithInBoundary(startIndex));

  auto beg = TimeUtil::currentTimeInNanos();

  uint64_t lastIndex = mLastIndex;

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
    auto &meta = getMeta(nextIndex);
    uint64_t nextLogSize = meta.length;

    if (lenInBytes + nextLogSize > maxLenInBytes) {
      break;
    }

    /// make sure about data safety
    verifyHMAC(meta);

    raft::LogEntry entry;

    void *entryAddr = getEntryAddr(meta.offset);
    if (!entry.ParseFromArray(entryAddr, meta.length)) {
      SPDLOG_WARN("failed to serialize LogEntry at {}", nextIndex);
      return 0;
    }

    ++batchSize;
    lenInBytes += nextLogSize;
    entries->push_back(std::move(entry));
  }

  auto end = TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("startIndex={}, batchSize={}, lenInBytes={}, timeCost={}ms",
              startIndex, batchSize, lenInBytes, (end - beg) / 1000000.0);
  return batchSize;
}

void Segment::truncateSuffix(uint64_t lastIndexKept) {
  if (lastIndexKept >= mLastIndex) {
    return;
  }

  auto beg = TimeUtil::currentTimeInNanos();

  /// update mLastIndex
  uint64_t prevLastIndex = mLastIndex;
  mLastIndex = lastIndexKept;

  /// update metaFile
  mMetaOffset = (mLastIndex - mFirstIndex + 1) * sizeof(LogMeta);
  void *metaAddr = reinterpret_cast<uint8_t *>(mMetaMemPtr) + mMetaOffset;

  uint64_t metaLen = (prevLastIndex - mLastIndex) * sizeof(LogMeta);

  ::memset(metaAddr, 0, metaLen);
  FileUtil::syncAt(metaAddr, metaLen);

  /// update dataFile
  const auto &meta = getMeta(mLastIndex);
  mDataOffset = meta.offset + meta.length;

  auto end = TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("Truncate suffix to {}, timeCost={}ms", mLastIndex, (end - beg) / 1000000.0);

  /// rename is needed for closed segment
  if (!mIsActive) {
    auto dataFromPath = mLogDir + dataFileNameForClosedSegment(mFirstIndex, prevLastIndex);
    auto dataToPath = mLogDir + dataFileNameForClosedSegment(mFirstIndex, mLastIndex);
    assert(::rename(dataFromPath.c_str(), dataToPath.c_str()) == 0);

    auto metaFromPath = mLogDir + metaFileNameForClosedSegment(mFirstIndex, prevLastIndex);
    auto metaToPath = mLogDir + metaFileNameForClosedSegment(mFirstIndex, mLastIndex);
    assert(::rename(metaFromPath.c_str(), metaToPath.c_str()) == 0);
  }
}

void Segment::dropSegment() const {
  auto tsInNano = TimeUtil::currentTimeInNanos();

  auto dataFromPath = mLogDir + (mIsActive ? dataFileNameForActiveSegment(mFirstIndex)
                                           : dataFileNameForClosedSegment(mFirstIndex, mLastIndex));
  auto dataToPath = dataFromPath + ".dropped." + std::to_string(tsInNano);

  assert(::rename(dataFromPath.c_str(), dataToPath.c_str()) == 0);

  auto metaFromPath = mLogDir + (mIsActive ? metaFileNameForActiveSegment(mFirstIndex)
                                           : metaFileNameForClosedSegment(mFirstIndex, mLastIndex));
  auto metaToPath = metaFromPath + ".dropped." + std::to_string(tsInNano);

  assert(::rename(metaFromPath.c_str(), metaToPath.c_str()) == 0);
}

void Segment::createHMAC(LogMeta *meta) const {
  auto *entryAddr = reinterpret_cast<const char *>(mDataMemPtr) + meta->offset;

  auto message = std::to_string(meta->index) + std::string(entryAddr, meta->length);
  auto digest = mCrypto->hmac(message);

  /// crypto disabled HMAC
  if (digest.empty()) {
    return;
  }

  assert(digest.size() == sizeof(meta->digest));
  ::memcpy(meta->digest, digest.c_str(), digest.size());
}

void Segment::verifyHMAC(const LogMeta &meta) const {
  std::string expectDigest(meta.digest, sizeof(meta.digest));

  /// no HMAC for this entry
  if (expectDigest == std::string(sizeof(meta.digest), '\0')) {
    return;
  }

  auto *entryAddr = static_cast<const char *>(getEntryAddr(meta.offset));
  auto message = std::to_string(meta.index) + std::string(entryAddr, meta.length);
  auto realDigest = mCrypto->hmac(message);

  /// crypto disabled HMAC
  if (realDigest.empty()) {
    return;
  }

  /// take effect iff this entry has HMAC and HMAC is enabled in crypto.
  assert(realDigest == expectDigest);
}

}  /// namespace storage
}  /// namespace gringofts

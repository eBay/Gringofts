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

#include <gtest/gtest.h>

#include "../../../../src/infra/raft/storage/Segment.h"
#include "../../../../src/infra/raft/storage/SegmentLog.h"
#include "../../../../src/infra/util/FileUtil.h"

namespace gringofts::storage::test {

class SegmentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mLogDir = "./logDir";
    Util::executeCmd("mkdir " + mLogDir);
  }
  void TearDown() override {
    Util::executeCmd("rm -rf " + mLogDir);
  }

  void createClosedSegments(uint64_t startIndex, uint64_t endIndex) {
    assert(endIndex > startIndex);
    auto crypto = std::make_shared<CryptoUtil>();
    Segment segment(mLogDir, startIndex, kMaxDataSize, kMaxMetaSize, crypto);
    for (auto index = startIndex; index <= endIndex; ++index) {
      raft::LogEntry entry;
      entry.mutable_version()->set_secret_key_version(1);
      entry.set_term(1);
      entry.set_index(index);
      entry.set_payload(std::string("abcd"));
      bool shouldRoll = segment.shouldRoll({entry});
      assert(!shouldRoll);
      segment.appendEntries({entry});
    }
    segment.closeActiveSegment();
  }

  std::string mLogDir;
  inline static const uint64_t kMaxDataSize = 10 * 4096;
  inline static const uint64_t kMaxMetaSize = 1 * 4096;
};

TEST_F(SegmentTest, basicOperations) {
  auto crypto = std::make_shared<CryptoUtil>();
  Segment segment(mLogDir, 1, kMaxDataSize, kMaxMetaSize, crypto);
  std::string result = Util::executeCmd("ls -l " + mLogDir);

  raft::LogEntry entry;
  entry.mutable_version()->set_secret_key_version(1);
  entry.set_term(1);
  entry.set_index(1);
  entry.set_payload(std::string(40949, 'a'));
  bool shouldRoll = segment.shouldRoll({entry});
  EXPECT_EQ(shouldRoll, true);

  entry.set_payload(std::string(40948, 'a'));
  shouldRoll = segment.shouldRoll({entry});
  EXPECT_EQ(shouldRoll, false);

  raft::LogEntry entryFetched;
  uint64_t termFetched;
  entry.set_payload(std::string(10, 'a'));
  segment.appendEntries({entry});
  EXPECT_EQ(segment.getFirstIndex(), 1);
  EXPECT_EQ(segment.getLastIndex(), 1);
  EXPECT_EQ(segment.getTerm(1, &termFetched), true);
  EXPECT_EQ(termFetched, 1);
  EXPECT_EQ(segment.getEntry(1, &entryFetched), true);
  EXPECT_EQ(entryFetched.index(), 1);
  EXPECT_EQ(entryFetched.payload(), std::string(10, 'a'));
  EXPECT_EQ(segment.getTerm(2, &termFetched), false);
  EXPECT_EQ(segment.getEntry(2, &entryFetched), false);

  entry.set_index(2);
  shouldRoll = segment.shouldRoll({entry});
  EXPECT_EQ(shouldRoll, false);
  segment.appendEntries({entry});
  EXPECT_EQ(segment.getLastIndex(), 2);

  entry.set_index(3);
  entry.set_payload(std::string(kMaxDataSize - 51, 'a'));
  shouldRoll = segment.shouldRoll({entry});
  EXPECT_EQ(shouldRoll, true);

  segment.truncateSuffix(1);
  EXPECT_EQ(segment.getLastIndex(), 1);
  shouldRoll = segment.shouldRoll({entry});
  EXPECT_EQ(shouldRoll, false);

  segment.closeActiveSegment();
  result = Util::executeCmd("ls -l " + mLogDir);
  EXPECT_NE(result.find("segment_1_1.data"), std::string::npos);

  segment.dropSegment();
  result = Util::executeCmd("ls -l " + mLogDir);
  EXPECT_NE(result.find("segment_1_1.data.dropped"), std::string::npos);
}

// This test case is to test recover a normal segment
TEST_F(SegmentTest, recoverSegment) {
  auto crypto = std::make_shared<CryptoUtil>();
  {
    // prepare
    Segment segment(mLogDir, 1, kMaxDataSize, kMaxMetaSize, crypto);

    raft::LogEntry entry;
    entry.mutable_version()->set_secret_key_version(1);
    entry.set_term(1);
    entry.set_index(1);
    entry.set_payload(std::string("abcdef"));
    segment.appendEntries({entry});
  }

  {
    // execute
    Segment segmentRecovered(mLogDir, 1, crypto);
    segmentRecovered.recoverActiveOrClosedSegment(kMaxDataSize, kMaxMetaSize);

    // check
    EXPECT_EQ(segmentRecovered.getFirstIndex(), 1);
    EXPECT_EQ(segmentRecovered.getLastIndex(), 1);
    raft::LogEntry entryFetched;
    uint64_t termFetched;
    EXPECT_EQ(segmentRecovered.getTerm(1, &termFetched), true);
    EXPECT_EQ(termFetched, 1);
    EXPECT_EQ(segmentRecovered.getEntry(1, &entryFetched), true);
    EXPECT_EQ(entryFetched.index(), 1);
    EXPECT_EQ(entryFetched.payload(), std::string("abcdef"));
  }

  {
    // check file size
    auto dataFd = ::open((std::string(mLogDir) + "/segment_in_progress_1.data").c_str(), O_RDWR);
    EXPECT_EQ(FileUtil::getFileSize(dataFd), kMaxDataSize);
    ::close(dataFd);
    auto metaFd = ::open((std::string(mLogDir) + "/segment_in_progress_1.meta").c_str(), O_RDWR);
    EXPECT_EQ(FileUtil::getFileSize(metaFd), kMaxMetaSize);
    ::close(metaFd);
  }
}

// This test case is to test recover a segment when data file size set fails
TEST_F(SegmentTest, recoverSegmentSetDataFileSizeFail) {
  // prepare
  auto crypto = std::make_shared<CryptoUtil>();
  {
    createClosedSegments(100, 120);
    FileUtil::setFileContentWithSync(mLogDir + "/first_index", std::to_string(100));
    auto dataPath = mLogDir + "/segment_in_progress_121.data";
    auto dataFd = ::open(dataPath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    ::close(dataFd);
  }

  // execute
  SegmentLog segmentLog(mLogDir, crypto, kMaxDataSize, kMaxMetaSize);

  // check
  auto result = Util::executeCmd("du -sb " + mLogDir + "/segment_in_progress_121.data");
  EXPECT_NE(result.find(std::to_string(kMaxDataSize)), std::string::npos);
  result = Util::executeCmd("du -sb " + mLogDir + "/segment_in_progress_121.meta");
  EXPECT_NE(result.find(std::to_string(kMaxMetaSize)), std::string::npos);
}

TEST_F(SegmentTest, recoverSegmentCreateMetaFileFail) {
  // prepare
  auto crypto = std::make_shared<CryptoUtil>();
  {
    createClosedSegments(100, 120);
    FileUtil::setFileContentWithSync(mLogDir + "/first_index", std::to_string(100));
    auto dataPath = mLogDir + "/segment_in_progress_121.data";
    auto dataFd = ::open(dataPath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    FileUtil::setFileSize(dataFd, kMaxDataSize);
    ::close(dataFd);
  }

  // execute
  SegmentLog segmentLog(mLogDir, crypto, kMaxDataSize, kMaxMetaSize);

  // check
  auto result = Util::executeCmd("du -sb " + mLogDir + "/segment_in_progress_121.data");
  EXPECT_NE(result.find(std::to_string(kMaxDataSize)), std::string::npos);
  result = Util::executeCmd("du -sb " + mLogDir + "/segment_in_progress_121.meta");
  EXPECT_NE(result.find(std::to_string(kMaxMetaSize)), std::string::npos);
}

TEST_F(SegmentTest, recoverSegmentSetMetaFileSizeFail) {
  // prepare
  auto crypto = std::make_shared<CryptoUtil>();
  {
    createClosedSegments(100, 120);
    FileUtil::setFileContentWithSync(mLogDir + "/first_index", std::to_string(100));
    auto dataPath = mLogDir + "/segment_in_progress_121.data";
    auto dataFd = ::open(dataPath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    FileUtil::setFileSize(dataFd, kMaxDataSize);
    ::close(dataFd);
    auto metaPath = mLogDir + "/segment_in_progress_121.meta";
    auto metaFd = ::open(metaPath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    auto metaFileSize = FileUtil::getFileSize(metaFd);
    EXPECT_EQ(metaFileSize, 0);
    ::close(metaFd);
  }

  // execute
  SegmentLog segmentLog(mLogDir, crypto, kMaxDataSize, kMaxMetaSize);

  // check
  auto result = Util::executeCmd("du -sb " + mLogDir + "/segment_in_progress_121.data");
  EXPECT_NE(result.find(std::to_string(kMaxDataSize)), std::string::npos);
  result = Util::executeCmd("du -sb " + mLogDir + "/segment_in_progress_121.meta");
  EXPECT_NE(result.find(std::to_string(kMaxMetaSize)), std::string::npos);
}

TEST_F(SegmentTest, recoverSmallSizeSegment) {
  auto crypto = std::make_shared<CryptoUtil>();
  {
    Segment segment(mLogDir, 1, kMaxDataSize, kMaxMetaSize, crypto);
    raft::LogEntry entry;
    entry.mutable_version()->set_secret_key_version(1);
    entry.set_term(1);
    entry.set_index(1);
    entry.set_payload(std::string("abcd"));
    segment.appendEntries({entry});
    entry.set_index(2);
    entry.set_payload(std::string("efgh"));
    segment.appendEntries({entry});
    auto result = Util::executeCmd("du -sb " + mLogDir + "/segment_in_progress_1.data");
    EXPECT_NE(result.find(std::to_string(kMaxDataSize)), std::string::npos);
    result = Util::executeCmd("du -sb " + mLogDir + "/segment_in_progress_1.meta");
    EXPECT_NE(result.find(std::to_string(kMaxMetaSize)), std::string::npos);
  }

  // execute
  SegmentLog segmentLog(mLogDir, crypto, kMaxDataSize * 2, kMaxMetaSize * 2);

  // check
  raft::LogEntry entryFetched;
  uint64_t termFetched;
  EXPECT_EQ(segmentLog.getTerm(1, &termFetched), true);
  EXPECT_EQ(termFetched, 1);
  EXPECT_EQ(segmentLog.getTerm(2, &termFetched), true);
  EXPECT_EQ(termFetched, 1);
  EXPECT_EQ(segmentLog.getEntry(1, &entryFetched), true);
  EXPECT_EQ(entryFetched.index(), 1);
  EXPECT_EQ(entryFetched.payload(), "abcd");
  EXPECT_EQ(segmentLog.getEntry(2, &entryFetched), true);
  EXPECT_EQ(entryFetched.index(), 2);
  EXPECT_EQ(entryFetched.payload(), "efgh");
  auto result = Util::executeCmd("du -sb " + mLogDir + "/segment_in_progress_1.data");
  EXPECT_NE(result.find(std::to_string(kMaxDataSize * 2)), std::string::npos);
  result = Util::executeCmd("du -sb " + mLogDir + "/segment_in_progress_1.meta");
  EXPECT_NE(result.find(std::to_string(kMaxMetaSize * 2)), std::string::npos);
}

TEST_F(SegmentTest, SerializedActiveEntriesSurviveOverwrite) {
  auto crypto = std::make_shared<CryptoUtil>();
  auto log = std::make_unique<SegmentLog>(mLogDir, crypto, kMaxDataSize, kMaxMetaSize);
  raft::LogEntry entry;
  entry.set_index(1);
  entry.set_term(1);
  entry.set_payload("first");
  ASSERT_TRUE(log->appendEntry(entry));
  entry.set_index(2);
  entry.set_payload(std::string(1024, '\xff'));
  ASSERT_TRUE(log->appendEntry(entry));
  const auto original = entry.SerializeAsString();

  std::vector<SerializedLogEntry> entries;
  ASSERT_EQ(log->getSerializedEntries(2, kMaxDataSize, 10, &entries), 1);
  log->truncateSuffix(1);
  entry.set_term(2);
  entry.set_payload(std::string(1024, '\0'));
  ASSERT_TRUE(log->appendEntriesByRef({&entry}));
  log.reset();
  ASSERT_EQ(entries.size(), 1);
  EXPECT_EQ(std::string(entries[0].mData.get(), entries[0].mSize), original);
}

TEST_F(SegmentTest, SerializedClosedEntriesRetainMappingAcrossTruncation) {
  auto crypto = std::make_shared<CryptoUtil>();
  auto log = std::make_unique<SegmentLog>(mLogDir, crypto, kMaxDataSize, kMaxMetaSize);
  raft::LogEntry entry;
  entry.set_term(1);
  entry.set_payload(std::string(10 * 1024, 'a'));
  for (uint64_t index = 1; index <= 3; ++index) {
    entry.set_index(index);
    if (index == 3) {
      entry.set_payload(std::string(30 * 1024, 'b'));
    }
    ASSERT_TRUE(log->appendEntry(entry));
  }
  std::vector<SerializedLogEntry> entries;
  ASSERT_EQ(log->getSerializedEntries(1, kMaxDataSize, 10, &entries), 2);
  std::weak_ptr<const char> owner = entries[0].mData;
  {
    std::vector<SerializedLogEntry> secondRead;
    ASSERT_EQ(log->getSerializedEntries(1, kMaxDataSize, 10, &secondRead), 2);
    EXPECT_EQ(entries[0].mData.get(), secondRead[0].mData.get());
  }
  const std::string original(entries[1].mData.get(), entries[1].mSize);
  log->truncateSuffix(1);
  entry.set_index(2);
  entry.set_term(2);
  ASSERT_TRUE(log->appendEntry(entry));
  log->truncatePrefix(3);
  log.reset();
  EXPECT_FALSE(owner.expired());
  EXPECT_EQ(std::string(entries[1].mData.get(), entries[1].mSize), original);
  raft::LogEntry parsed;
  ASSERT_TRUE(parsed.ParseFromArray(entries[1].mData.get(), entries[1].mSize));
  EXPECT_EQ(parsed.term(), 1);
  EXPECT_EQ(parsed.index(), 2);
  entries.clear();
  EXPECT_TRUE(owner.expired());
}

TEST_F(SegmentTest, SerializedBatchLimitsAndHeartbeat) {
  auto crypto = std::make_shared<CryptoUtil>();
  SegmentLog log(mLogDir, crypto, kMaxDataSize, kMaxMetaSize);
  raft::LogEntry entry;
  entry.set_index(1);
  entry.set_term(1);
  entry.set_payload(std::string(1024, '\0'));
  ASSERT_TRUE(log.appendEntry(entry));
  const auto firstSize = entry.ByteSizeLong();
  entry.set_index(2);
  ASSERT_TRUE(log.appendEntry(entry));
  std::vector<SerializedLogEntry> entries;
  EXPECT_EQ(log.getSerializedEntries(1, firstSize - 1, 10, &entries), 0);
  EXPECT_TRUE(entries.empty());
  EXPECT_EQ(log.getSerializedEntries(1, firstSize, 10, &entries), 1);
  EXPECT_EQ(log.getSerializedEntries(1, kMaxDataSize, 1, &entries), 1);
  EXPECT_EQ(log.getSerializedEntries(3, kMaxDataSize, 10, &entries), 0);
  EXPECT_TRUE(entries.empty());
  EXPECT_EQ(log.getSerializedEntries(1, kMaxDataSize, 0, &entries), 0);
  EXPECT_TRUE(entries.empty());
}

TEST_F(SegmentTest, SerializedReadsValidateNestedMessagesWithoutParsingPayload) {
  auto crypto = std::make_shared<CryptoUtil>();
  Segment segment(mLogDir, 1, kMaxDataSize, kMaxMetaSize, crypto);
  raft::LogEntry entry;
  entry.set_index(1);
  entry.set_payload(std::string("\x80\xff\0", 3));
  entry.mutable_version()->set_secret_key_version(1);
  entry.mutable_specialtag()->set_identifier(1);
  segment.appendEntriesByRef({&entry});
  std::vector<std::string_view> entries;
  ASSERT_EQ(segment.getSerializedEntries(1, kMaxDataSize, 10, &entries), 1);
  const auto original = entry.SerializeAsString();
  auto *data = const_cast<char *>(entries[0].data());
  for (const auto &marker : {std::string("\x32\x02\x08\x01", 4), std::string("\x3a\x02\x08\x01", 4)}) {
    auto offset = original.find(marker);
    ASSERT_NE(offset, std::string::npos);
    data[offset + 3] = '\x80';
    EXPECT_EQ(segment.getSerializedEntries(1, kMaxDataSize, 10, &entries), 0);
    EXPECT_TRUE(entries.empty());
    data[offset + 3] = original[offset + 3];
    data[offset + 1] = '\x7f';
    EXPECT_EQ(segment.getSerializedEntries(1, kMaxDataSize, 10, &entries), 0);
    data[offset + 1] = original[offset + 1];
  }
  EXPECT_EQ(segment.getSerializedEntries(1, kMaxDataSize, 10, &entries), 1);
}

}  /// namespace gringofts::storage::test

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

#include "../../../../src/infra/raft/storage/Log.h"
#include "../../../../src/infra/raft/storage/InMemoryLog.h"
#include "../../../../src/infra/raft/storage/SegmentLog.h"

namespace gringofts::storage::test {

class LogTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}

  void everythingAboutLog(Log *log);

  static const SecKeyVersion defaultVersion = 1;
};

void LogTest::everythingAboutLog(Log *log) {
  /// appendEntry and appendEntries
  {
    std::string str10KiB(10 * 1024, 'a');
    std::string str4KiB(4 * 1024, 'a');

    raft::LogEntry entry1;
    entry1.mutable_version()->set_secret_key_version(log->getLatestSecKeyVersion());
    entry1.set_index(1);
    entry1.set_payload(str10KiB);

    raft::LogEntry entry2;
    entry2.mutable_version()->set_secret_key_version(log->getLatestSecKeyVersion());
    entry2.set_index(2);
    entry2.set_payload(str4KiB);

    raft::LogEntry entry3;
    entry3.mutable_version()->set_secret_key_version(log->getLatestSecKeyVersion());
    entry3.set_index(3);
    entry3.set_payload(str4KiB);

    EXPECT_FALSE(log->appendEntry(entry2));
    EXPECT_TRUE(log->appendEntry(entry1));

    EXPECT_FALSE(log->appendEntries({entry3, entry2}));
    EXPECT_TRUE(log->appendEntries({entry2, entry3}));
  }

  /// for deprecated interface, will delete later
  {
    std::string str4KiB(4 * 1024, 'a');

    raft::LogEntry entry4;
    entry4.mutable_version()->set_secret_key_version(log->getLatestSecKeyVersion());
    entry4.set_index(4);
    entry4.set_payload(str4KiB);

    raft::LogEntry entry5;
    entry5.mutable_version()->set_secret_key_version(log->getLatestSecKeyVersion());
    entry5.set_index(5);
    entry5.set_payload(str4KiB);

    log->append(entry4);
    log->append(std::vector<raft::LogEntry>{entry5});

    EXPECT_EQ(log->getLogTerm(4), 0);
  }

  /// getFirstLogIndex and getLastLogIndex
  EXPECT_EQ(log->getFirstLogIndex(), 1);
  EXPECT_EQ(log->getLastLogIndex(), 5);

  /// getEntry and getTerm
  {
    raft::LogEntry entry;
    uint64_t term = 0;

    EXPECT_TRUE(log->getEntry(1, &entry));
    EXPECT_TRUE(log->getTerm(1, &term));

    EXPECT_FALSE(log->getEntry(6, &entry));
    EXPECT_FALSE(log->getTerm(6, &term));
  }

  /// getEntries
  {
    std::vector<raft::LogEntry> entries;
    EXPECT_EQ(log->getEntries(1, 12 * 1024, 10, &entries), 1);
  }

  /// truncatePrefix and truncateSuffix
  log->truncatePrefix(0);
  log->truncatePrefix(2);

  log->truncateSuffix(6);
  log->truncateSuffix(2);

  /// voteFor and currentTerm
  log->setCurrentTerm(10);
  EXPECT_EQ(log->getCurrentTerm(), 10);

  log->setVoteFor(10);
  EXPECT_EQ(log->getVote(), 10);

  /// misc
  EXPECT_NE(log->getName(), "");
}

TEST_F(LogTest, SegmentLogTest) {
  /// setup
  std::string logDir = "./logDir";
  uint64_t segmentDataSizeLimit = 16 * 1024;
  uint64_t segmentMetaSizeLimit = 16 * 1024;

  Util::executeCmd("mkdir " + logDir);

  /// init
  auto crypto = std::make_shared<CryptoUtil>();
  auto log = std::make_unique<SegmentLog>(logDir, crypto,
                                          segmentDataSizeLimit, segmentMetaSizeLimit);

  /// basic ops
  everythingAboutLog(log.get());

  /// destroy and re-open
  log.reset();
  log = std::make_unique<SegmentLog>(logDir, crypto,
                                     segmentDataSizeLimit, segmentMetaSizeLimit);

  /// teardown
  Util::executeCmd("rm -rf " + logDir);
}

TEST_F(LogTest, HMACTest) {
  /// setup
  std::string logDir = "./logDir";
  uint64_t segmentDataSizeLimit = 16 * 1024;
  uint64_t segmentMetaSizeLimit = 16 * 1024;

  Util::executeCmd("mkdir " + logDir);

  /// init
  auto cryptoDisableHMAC = std::make_shared<CryptoUtil>();

  auto cryptoEnableHMAC = std::make_shared<CryptoUtil>();
  cryptoEnableHMAC->init(defaultVersion, "01234567890123456789012345678901");

  std::string str4KiB(4 * 1024, 'a');

  /// append 10 entries meanwhile disable HMAC
  auto log = std::make_unique<SegmentLog>(logDir, cryptoDisableHMAC,
                                          segmentDataSizeLimit, segmentMetaSizeLimit);
  for (auto i = 1; i <= 10; ++i) {
    raft::LogEntry entry;
    entry.mutable_version()->set_secret_key_version(log->getLatestSecKeyVersion());
    entry.set_index(i);
    entry.set_payload(str4KiB);
    log->appendEntry(entry);
  }

  EXPECT_EQ(log->getLastLogIndex(), 10);

  /// destroy and re-open
  log.reset();
  log = std::make_unique<SegmentLog>(logDir, cryptoEnableHMAC,
                                     segmentDataSizeLimit, segmentMetaSizeLimit);

  /// append 10 entries meanwhile enable HMAC
  for (auto i = 11; i <= 20; ++i) {
    raft::LogEntry entry;
    entry.mutable_version()->set_secret_key_version(log->getLatestSecKeyVersion());
    entry.set_index(i);
    entry.set_payload(str4KiB);
    log->appendEntry(entry);
  }

  EXPECT_EQ(log->getLastLogIndex(), 20);

  /// read entries
  raft::LogEntry entry;
  /// HMAC disable
  EXPECT_TRUE(log->getEntry(10, &entry));
  /// HMAC enable
  EXPECT_TRUE(log->getEntry(20, &entry));

  /// teardown
  Util::executeCmd("rm -rf " + logDir);
}

TEST_F(LogTest, InMemoryLogTest) {
  /// init
  auto log = std::make_unique<InMemoryLog>();

  /// basic ops
  everythingAboutLog(log.get());

  /// specific op for in memory log
  {
    raft::LogEntry entry;
    uint64_t term;

    EXPECT_TRUE(log->getEntry(0, &entry));
    EXPECT_TRUE(log->getTerm(0, &term));

    std::vector<raft::LogEntry> entries;
    entries.resize(1);

    EXPECT_TRUE(log->getEntries(2, 1, &entries[0]));
  }
}

}  /// namespace gringofts::storage::test

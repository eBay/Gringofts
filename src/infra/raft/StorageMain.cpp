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

#include <stdlib.h>
#include <time.h>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>

#include "../util/RandomUtil.h"
#include "storage/InMemoryLog.h"
#include "storage/SegmentLog.h"

using gringofts::RandomUtil;
using gringofts::raft::LogEntry;
using gringofts::storage::InMemoryLog;
using gringofts::storage::SegmentLog;

constexpr uint64_t kRecoveryInterval = 10 * 1000;
constexpr uint64_t kTruncateInterval = 20 * 1000;

std::string getRandomPayload() {
  constexpr uint64_t kAlphabetNum = 26;
  constexpr uint64_t kBaseLen = 1000;
  constexpr uint64_t kVarLen = 2000;

  std::string payload;

  uint64_t length = RandomUtil::randomRange(kBaseLen, kBaseLen + kVarLen);
  payload.resize(length, ' ');

  for (uint64_t i = 0; i < length; ++i) {
    payload[i] = 'A' + RandomUtil::randomRange(0, kAlphabetNum - 1);
  }

  return payload;
}

void storageABTest(uint64_t runCount, const std::string &logDir,
                   uint64_t dataSizeLimit, uint64_t metaSizeLimit) {
  auto log1 = std::make_unique<InMemoryLog>();

  auto crypto = std::make_shared<gringofts::CryptoUtil>();
  auto log2 = std::make_unique<SegmentLog>(logDir, crypto, dataSizeLimit, metaSizeLimit);

  SPDLOG_INFO("abTest: {} vs {}", log1->getName(), log2->getName());

  std::vector<LogEntry> entries;

  for (uint64_t i = 1; i <= runCount; ++i) {
    LogEntry entry;

    entry.mutable_version()->set_secret_key_version(crypto->getLatestSecKeyVersion());
    entry.set_term(i);
    entry.set_payload(getRandomPayload());

    entries.push_back(std::move(entry));

    /// recovery segment log
    if (i % kRecoveryInterval == 0) {
      log2 = std::make_unique<SegmentLog>(logDir, crypto, dataSizeLimit, metaSizeLimit);
    }

    /** 4 different operation */
    auto randomOp = RandomUtil::randomRange(1, 4);
    switch (randomOp) {
      case 1: {   /// write
        assert(log1->getFirstLogIndex() == log2->getFirstLogIndex());
        assert(log1->getLastLogIndex() == log2->getLastLogIndex());

        uint64_t lastIndex = log1->getLastLogIndex();
        for (auto &e : entries) {
          e.set_index(++lastIndex);
        }

        log1->append(entries);
        log2->append(entries);

        entries.clear();
        break;
      }
      case 2: {   /// read
        assert(log1->getFirstLogIndex() == log2->getFirstLogIndex());
        assert(log1->getLastLogIndex() == log2->getLastLogIndex());

        auto firstIndex = log1->getFirstLogIndex();
        auto lastIndex = log1->getLastLogIndex();

        if (lastIndex == firstIndex - 1) {
          /// empty storage
          break;
        }

        uint64_t index = RandomUtil::randomRange(firstIndex, lastIndex);

        LogEntry entry1;
        LogEntry entry2;
        assert(log1->getEntry(index, &entry1));
        assert(log2->getEntry(index, &entry2));
        assert(entry1.payload() == entry2.payload());
        break;
      }
      case 3 : {   /// truncate suffix
        assert(log1->getFirstLogIndex() == log2->getFirstLogIndex());
        assert(log1->getLastLogIndex() == log2->getLastLogIndex());

        auto firstIndex = log1->getFirstLogIndex();
        auto lastIndex = log1->getLastLogIndex();

        if (lastIndex == firstIndex - 1 || lastIndex - firstIndex < kTruncateInterval) {
          /// Avoid frequently truncate
          break;
        }

        uint64_t lastIndexKept = RandomUtil::randomRange(firstIndex, lastIndex);

        log1->truncateSuffix(lastIndexKept);
        log2->truncateSuffix(lastIndexKept);
        break;
      }
      case 4: {   /// truncate prefix
        assert(log1->getFirstLogIndex() == log2->getFirstLogIndex());
        assert(log1->getLastLogIndex() == log2->getLastLogIndex());

        auto firstIndex = log1->getFirstLogIndex();
        auto lastIndex = log1->getLastLogIndex();

        if (lastIndex == firstIndex - 1 || lastIndex - firstIndex < kTruncateInterval) {
          /// Avoid frequently truncate
          break;
        }

        uint64_t firstIndexKept = RandomUtil::randomRange(firstIndex, lastIndex);

        log1->truncatePrefix(firstIndexKept);
        log2->truncatePrefix(firstIndexKept);
        break;
      }
      default : {
        assert(false);
      }
    }
  }

  /// full compare
  assert(log1->getFirstLogIndex() == log2->getFirstLogIndex());
  assert(log1->getLastLogIndex() == log2->getLastLogIndex());

  auto firstIndex = log1->getFirstLogIndex();
  auto lastIndex = log1->getLastLogIndex();

  SPDLOG_INFO("firstIndex={}, lastIndex={}", firstIndex, lastIndex);

  for (uint64_t i = firstIndex; i <= lastIndex; ++i) {
    LogEntry entry1;
    LogEntry entry2;
    assert(log1->getEntry(i, &entry1));
    assert(log2->getEntry(i, &entry2));
    assert(entry1.payload() == entry2.payload());
  }
}

int main(int argc, char *argv[]) {
  ::srand(::time(NULL));

  spdlog::stdout_logger_mt("console");
  spdlog::set_pattern("[%H:%M:%S.%F] [%s:%# %!] [%l] [thread %t] %v");

  uint64_t runCount = 200 * 1000;
  std::string logDir = "./data";
  uint64_t dataSizeLimit = 2 << 23;   /// 8M
  uint64_t metaSizeLimit = 2 << 19;   /// 512K

  storageABTest(runCount, logDir, dataSizeLimit, metaSizeLimit);
  return 0;
}

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

#ifndef SRC_INFRA_ES_STORE_SNAPSHOTUTIL_H_
#define SRC_INFRA_ES_STORE_SNAPSHOTUTIL_H_

#include <optional>

#include <spdlog/spdlog.h>

#include "../../util/CryptoUtil.h"
#include "../../util/FileUtil.h"

#include "../CommandDecoder.h"
#include "../EventDecoder.h"
#include "../StateMachine.h"

namespace gringofts {

class SnapshotUtil {
 public:
  static constexpr auto &kSnapshotSuffix = ".snapshot";
  static constexpr auto &kSnapshotSuffixRegex = ".snapshot$";

  /**
   * Take the snapshot of the state machine and persist it to local disk
   * @param currentOffset snapshot = apply events in [0, currentOffset]
   * @param snapshotDir the directory where the snapshot will be saved to
   * @param stateMachine the state machine whose state will be restored via the snapshot
   * @param crypto the instance used to encrypt snapshot
   * @return <true, path of snapshot file> if succeed, false otherwise
   */
  template<typename StateMachineType>
  static std::pair<bool, std::string> takeSnapshotAndPersist(uint64_t currentOffset,
                                                             const std::string &snapshotDir,
                                                             const StateMachineType &stateMachine,
                                                             CryptoUtil &crypto) noexcept {  // NOLINT [runtime/references]
    auto offset = currentOffset;
    const auto &snapshotFilePath = snapshotDir + "/" + std::to_string(offset) + kSnapshotSuffix;
    const auto &tmpSnapshotFilePath = snapshotFilePath + ".tmp";
    std::ofstream ofile{tmpSnapshotFilePath, std::ios::trunc | std::ios::binary};
    if (!ofile) {
      SPDLOG_WARN("Failed to create a snapshot file {} due to errno: {}", tmpSnapshotFilePath, errno);
      return std::make_pair(false, snapshotFilePath);
    }
    SPDLOG_INFO("Start taking snapshot");
    auto result = stateMachine.createSnapshotAndPersist(offset, ofile, crypto);
    ofile.close();
    FileUtil::checkFileState(ofile);

    SPDLOG_INFO("Start fsyncing snapshot file to disk");
    FileUtil::syncFile(tmpSnapshotFilePath);

    /// the temp file is saved properly and not corrupted, so can be safely renamed
    auto ret = ::rename(tmpSnapshotFilePath.c_str(), snapshotFilePath.c_str());
    if (ret != 0) {
      SPDLOG_WARN("Failed to rename the temporary snapshot file {} to {} due to errno: {}",
                  tmpSnapshotFilePath,
                  snapshotFilePath,
                  errno);
      return std::make_pair(false, snapshotFilePath);
    }

    SPDLOG_INFO("Snapshot is taken");
    return std::make_pair(result, snapshotFilePath);
  }

  /**
   * Load the snapshot file to restore state machine's state
   * @param snapshotDir the directory where the snapshot will be saved to
   * @param stateMachine the state machine whose state will be restored via the snapshot
   * @param commandDecoder decoder used to decode command
   * @param eventDecoder decoder used to decode event
   * @param crypto the instance used to decrypt snapshot
   * @return the read offset in the snapshot if succeed, std::nullopt otherwise
   */
  template<typename StateMachineType>
  static std::optional<uint64_t> loadLatestSnapshot(const std::string &snapshotDir,
                                                    StateMachineType &stateMachine,  // NOLINT [runtime/references]
                                                    const CommandDecoder &commandDecoder,
                                                    const EventDecoder &eventDecoder,
                                                    CryptoUtil &crypto) noexcept {  // NOLINT [runtime/references]
    SPDLOG_INFO("Figuring out latest snapshot file under {}", snapshotDir);
    auto snapshotFileOpt = getLargestSnapshot(snapshotDir);
    if (!snapshotFileOpt || (*snapshotFileOpt).empty()) {
      SPDLOG_INFO("No snapshot files are found under {}", snapshotDir);
      return std::nullopt;
    }

    const auto &snapshotFilePath = snapshotDir + "/" + snapshotFileOpt.value();
    SPDLOG_INFO("Loading snapshot file {}", snapshotFilePath);
    std::ifstream ifile{snapshotFilePath, std::ios::binary};
    if (!ifile) {
      SPDLOG_WARN("Failed to open a snapshot file {} due to errno: {}", snapshotFilePath, errno);
      return std::nullopt;
    }
    const auto &offset = stateMachine.loadSnapshotFrom(ifile, commandDecoder, eventDecoder, crypto);
    ifile.close();
    SPDLOG_INFO("Snapshot is loaded");

    return offset;
  }

  /**
   * Given snapshot Dir, return offset of latest snapshot
   */
  static std::optional<uint64_t> findLatestSnapshotOffset(const std::string &snapshotDir) noexcept {
    auto snapshotFileOpt = getLargestSnapshot(snapshotDir);
    if (!snapshotFileOpt || (*snapshotFileOpt).empty()) {
      SPDLOG_INFO("No snapshot files are found under {}", snapshotDir);
      return std::nullopt;
    }

    std::string fileName = *snapshotFileOpt;
    SPDLOG_INFO("Find latest snapshot file: {} under dir: {}", fileName, snapshotDir);

    fileName.resize(fileName.size() - std::string(kSnapshotSuffix).size());
    uint64_t offset = std::stoull(fileName);
    return offset;
  }

  /**
   * Given checkpoint Dir, return offset of latest checkpoint
   */
  static std::optional<uint64_t> findLatestCheckpointOffset(const std::string &checkpointDir) noexcept {
    std::regex checkpointRegex("([0-9]+)\\.[0-9]+\\.checkpoint$");
    std::smatch checkpointMatch;

    int64_t largestIndex = -1;
    auto dirNames = FileUtil::listDirs(checkpointDir);

    for (const auto &dirName : dirNames) {
      if (std::regex_search(dirName, checkpointMatch, checkpointRegex)) {
        int64_t index = std::stoll(checkpointMatch[1]);

        SPDLOG_INFO("dir {} match checkpoint pattern by {}, with index {}.",
                    dirName, checkpointMatch.str(0), index);

        if (index > largestIndex) {
          largestIndex = index;
        }
      }
    }

    return largestIndex != -1 ? std::optional<uint64_t>(largestIndex)
                              : std::nullopt;
  }

 private:
  /**
   * Get the largest snapshot under the target dir
   */
  static std::optional<std::string> getLargestSnapshot(const std::string &dir) noexcept {
    std::regex snapshotRegex("([0-9]+).snapshot$");
    std::smatch snapshotMatch;

    int64_t largestIndex = -1;
    std::string largestSnapshot;

    auto fileNames = FileUtil::listFiles(dir);

    for (const auto &fileName : fileNames) {
      if (std::regex_search(fileName, snapshotMatch, snapshotRegex)) {
        int64_t index = std::stoll(snapshotMatch[1]);
        SPDLOG_INFO("file {} match snapshot pattern by {}, with index {}.",
                    fileName, snapshotMatch.str(0), index);

        if (index > largestIndex) {
          largestIndex = index;
          largestSnapshot = snapshotMatch.str(0);
        }
      }
    }
    return largestIndex != -1 ? std::optional<std::string>(largestSnapshot)
                              : std::nullopt;
  }
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_STORE_SNAPSHOTUTIL_H_

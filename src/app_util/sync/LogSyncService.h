/************************************************************************
Copyright 2019-2021 eBay Inc.
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

#ifndef SRC_APP_UTIL_SYNC_LOGSYNCSERVICE_H_
#define SRC_APP_UTIL_SYNC_LOGSYNCSERVICE_H_

#include <atomic>
#include <mutex>


#include "LogReader.h"
#include "../control/split/SplitCommand.h"
#include "../../infra/raft/RaftInterface.h"

namespace gringofts::app::sync {

using raft::RaftInterface;

class LogSyncService {
 public:
  LogSyncService(std::shared_ptr<RaftInterface> raft, std::optional<TlsConf> tls,
                 std::unique_ptr<ReaderFactory> factory = std::make_unique<LogReaderFactoryImp>());

  inline uint64_t lastIndex() const { return mLastIndex; }

  inline bool synced() const { return mEndLogFlag; }

  inline void lastLogEntry(LogEntry *log) { mRaft->getEntry(mLastIndex, log); }

  void setTargets(std::vector<std::string> endPoints, std::string planId);

  bool terminateSyncMode(const std::string &planId, uint64_t tagCommitIndex, std::string *errMsg);

  void run();

  virtual ~LogSyncService();


 private:
  bool endLog(const LogEntry &logEntry) const;

  bool truncateLogs(std::vector<LogEntry> *logs) const;

 private:
  std::unique_ptr<ReaderFactory> mFactory;
  std::vector<std::unique_ptr<LogReader>> mReaders;
  std::string mPlanId;
  std::shared_ptr<raft::RaftInterface> mRaft;
  uint64_t mLastIndex;
  std::thread mThread;
  std::atomic_bool mRunning;
  bool mEndLogFlag;
  std::mutex mReaderMutex;
  std::condition_variable mReaderCV;
  std::optional<TlsConf> mTlsConf;
};

}  // namespace gringofts::app::sync

#endif  // SRC_APP_UTIL_SYNC_LOGSYNCSERVICE_H_

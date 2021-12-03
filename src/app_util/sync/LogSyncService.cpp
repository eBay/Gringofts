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

#include "LogSyncService.h"

#include <spdlog/spdlog.h>
#include <absl/strings/str_format.h>
#include "../../infra/raft/RaftSignal.h"


namespace gringofts::app::sync {

LogSyncService::LogSyncService(std::shared_ptr<RaftInterface> raft, std::optional<TlsConf> tls,
                               std::unique_ptr<ReaderFactory> factory)
    : mRaft(raft), mLastIndex(raft->getLastLogIndex()), mRunning(true),
      mEndLogFlag(false), mTlsConf(std::move(tls)), mFactory(std::move(factory)) {
  mThread = std::thread([this]() { run(); });
  SPDLOG_INFO("init log sync service with lastIndex:{}, tls:{}", mLastIndex, mTlsConf.has_value());
}

void LogSyncService::setTargets(std::vector<std::string> endPoints, std::string planId) {
  std::unique_lock lk{mReaderMutex};
  mPlanId = std::move(planId);
  if (mLastIndex != 0) {
    LogEntry lastEntry;
    mRaft->getEntry(mLastIndex, &lastEntry);
    if (endLog(lastEntry)) {
      SPDLOG_INFO("find end split event log with planId {}", mPlanId);
      mEndLogFlag = true;
      return;
    }
  }
  if (mEndLogFlag) {
    if (mThread.joinable()) {
      mThread.join();
    }
    SPDLOG_INFO("Thread has been stopped since synced tag before, need to restart syncing thread");
    mEndLogFlag = false;
    mThread = std::thread([this]() { run(); });
  }
  mReaders.clear();
  for (const auto &endPoint : endPoints) {
    mReaders.push_back(mFactory->create(endPoint, mTlsConf));
    SPDLOG_INFO("init reader for {}", endPoint);
    mReaders.back()->connect();
  }
  mReaderCV.notify_one();
}

void LogSyncService::run() {
  pthread_setname_np(pthread_self(), "SyncLogThread");
  uint64_t clientIdx = 0;
  while (mRunning) {
    if (mEndLogFlag) {
      SPDLOG_INFO("all logs before(include) split has been synced");
      break;
    }
    FetchStatus r;
    std::vector<LogEntry> logs;
    {
      std::unique_lock lk{mReaderMutex};
      mReaderCV.wait(lk, [this]() { return !mReaders.empty() || !mRunning; });
      if (!mRunning) {
        SPDLOG_INFO("finish running");
        break;
      }
      clientIdx %= mReaders.size();
      auto &reader = mReaders[clientIdx];
      r = reader->fetch(mLastIndex + 1, &logs);
    }
    if (r == FetchStatus::OK) {
      mEndLogFlag = truncateLogs(&logs);
      if (!logs.empty()) {
        auto endIndex = mLastIndex + logs.size();
        mRaft->enqueueSyncRequest({std::move(logs), mLastIndex + 1, endIndex, std::nullopt});
        mLastIndex = endIndex;
      }
    } else {
      using std::chrono_literals::operator ""s;
      std::this_thread::sleep_for(1s);
      clientIdx++;
      clientIdx %= mReaders.size();
      SPDLOG_INFO("switch to client {}", clientIdx);
    }
  }
}

LogSyncService::~LogSyncService() {
  SPDLOG_INFO("log sync service bye bye");
  mRunning = false;
  mReaderCV.notify_one();
  if (mThread.joinable()) {
    mThread.join();
  }
}

bool LogSyncService::endLog(const LogEntry &logEntry) const {
  if (logEntry.has_specialtag() && logEntry.specialtag().identifier() == ctrl::split::SPILT_COMMAND) {
    if (logEntry.specialtag().payload() == mPlanId) {
      return true;
    } else {
      SPDLOG_WARN("find split plan id {}, but we are waiting for {}",
                  logEntry.specialtag().payload(), mPlanId);
    }
  }
  return false;
}

bool LogSyncService::truncateLogs(std::vector<LogEntry> *logs) const {
  assert(!mEndLogFlag);
  bool truncated = false;
  for (auto it = logs->begin(); it != logs->end();) {
    if (truncated) {
      it = logs->erase(it);
      continue;
    }
    if (endLog(*it)) {
      SPDLOG_INFO("find split log with planId {}, skip later logs", mPlanId);
      truncated = true;
    }
    it++;
  }
  return truncated;
}

bool LogSyncService::terminateSyncMode(const std::string &planId, uint64_t tagCommitIndex, std::string *errMsg) {
  if (!synced()) {
    *errMsg = "still syncing logs";
    return false;
  }
  if (tagCommitIndex != 0 && tagCommitIndex != lastIndex()) {
    *errMsg = absl::StrFormat("last index should be %d", lastIndex());
    return false;
  }
  LogEntry log;
  lastLogEntry(&log);
  if (!log.has_specialtag()) {
    *errMsg = "last index log has no split Tag";
    return false;
  }
  if (log.specialtag().payload() != planId) {
    *errMsg = absl::StrFormat("split Tag planId is %s, but we want %s", log.specialtag().payload(), planId);
    return false;
  }
  SPDLOG_INFO("check passed, start to stop sync mode");
  using gringofts::Signal;
  using gringofts::raft::StopSyncRoleSignal;
  // lastIndex is last one from old cluster
  // so init index is lastIndex + 1
  Signal::hub << std::make_shared<StopSyncRoleSignal>(lastIndex() + 1);
  return true;
}

}  // namespace gringofts::app::sync

/************************************************************************
Copyright 2021-2022 eBay Inc.
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


#include "ReplyLoop.h"

#include <infra/common_types.h>
#include <spdlog/spdlog.h>

#include "../utils/TimeUtil.h"

namespace goblin::kvengine::raft {

ReplyLoop::ReplyLoop(
    const std::shared_ptr<store::VersionStore> &versionStore,
    const std::shared_ptr<gringofts::raft::RaftInterface> &raftImpl)
    : mVersionStore(versionStore),
    mRaftImpl(raftImpl),
    mTaskCountGauge(gringofts::getGauge("reply_queue_size", {})),
    mSuccessWriteCounter(gringofts::getCounter("success_write_count", {})),
    mSuccessReadCounter(gringofts::getCounter("success_read_count", {})) {
  mPopThread = std::thread(&ReplyLoop::popThreadMain, this);

  for (uint64_t i = 0; i < mConcurrency; ++i) {
    mReplyThreads.emplace_back(&ReplyLoop::replyThreadMain, this);
  }

  SPDLOG_INFO("ReplyLoop started, Concurrency={}.", mConcurrency);
}

ReplyLoop::~ReplyLoop() {
  mRunning = false;

  if (mPopThread.joinable()) {
    mPopThread.join();
  }

  for (auto &t : mReplyThreads) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void ReplyLoop::pushTask(
    uint64_t index,
    uint64_t term,
    const store::VersionType &version,
    std::shared_ptr<model::CommandContext> context) {

  /// TODO: set up a metric command
  /// metrics, before raft commit
  context->setBeforeRaftCommitTimeInNanos();
  SPDLOG_INFO("debug: index {}, term {}, reply queue {}", index, term, mTaskCount.load());
  /// report metrics in this single thd loop
  context->reportMetrics();

  auto taskPtr = std::make_shared<Task>();
  taskPtr->mIndex = index;
  taskPtr->mTerm = term;
  taskPtr->mVersion = version;
  taskPtr->mContext = context;
  mTaskCount += 1;

  /// write lock
  std::unique_lock<std::shared_mutex> lock(mMutex);
  mTaskQueue.push_back(std::move(taskPtr));
}

void ReplyLoop::popThreadMain() {
  pthread_setname_np(pthread_self(), "ReplyLoop_pop");

  while (mRunning) {
    bool busy = false;

    mTaskCountGauge.set(mTaskCount.load());
    {
      /// read lock
      std::shared_lock<std::shared_mutex> lock(mMutex);
      if (!mTaskQueue.empty() && mTaskQueue.front()->mFlag == 2) {
        busy = true;
      }
    }

    if (busy) {
      /// write lock
      std::unique_lock<std::shared_mutex> lock(mMutex);
      /// update metrics
      auto summary = gringofts::getSummary("request_overall_latency_in_ms", {});
      auto latency = mTaskQueue.front()->mOverallLatency / 1000000.0;
      /// SPDLOG_INFO("debug: show: {}", latency);
      summary.observe(latency);
      mTaskQueue.pop_front();
    } else {
      usleep(1000);   /// nothing to do, sleep 1ms
    }
  }
}

void ReplyLoop::replyThreadMain() {
  pthread_setname_np(pthread_self(), "ReplyLoop_reply");

  while (mRunning) {
    TaskPtr taskPtr;

    {
      /// read lock
      std::shared_lock<std::shared_mutex> lock(mMutex);

      for (auto &currPtr : mTaskQueue) {
        uint64_t expected = 0;
        if (currPtr->mFlag.compare_exchange_strong(expected, 1)) {
          taskPtr = currPtr;
          break;
        }
      }
    }

    if (taskPtr) {
      replyTask(taskPtr.get());
      mTaskCount -= 1;
    } else {
      usleep(1000);   /// nothing to do, sleep 1ms
    }
  }
}

bool ReplyLoop::waitTillCommittedOrQuit(uint64_t index, uint64_t term) {
  /**
   * compare <lastLogIndex, currentTerm> with <index, term>
   *
   *   <  , ==   same leader, keep waiting
   *
   *   >= , ==   same leader, keep waiting
   *
   *   <  , >    as new leader, quit
   *             as new follower, quit
   *
   *   >= , >    as new leader, keep waiting, will help commit entries from old leader
   *             as new follower, keep waiting, new leader might help commit
   */

  while (mRunning) {
    uint64_t term1 = 0;
    uint64_t term2 = 0;
    uint64_t lastLogIndex = 0;

    do {
      term1 = mRaftImpl->getCurrentTerm();
      lastLogIndex = mRaftImpl->getLastLogIndex();
      term2 = mRaftImpl->getCurrentTerm();
    } while (term1 != term2);

    assert(term1 >= term);

    if (lastLogIndex < index && term1 > term) {
      return false;   /// we can quit
    }

    if (mRaftImpl->getCommitIndex() < index) {
      usleep(1000);   /// sleep 1ms, retry
      continue;
    }

    gringofts::raft::LogEntry entry;
    assert(mRaftImpl->getEntry(index, &entry));
    return entry.term() == term;
  }

  SPDLOG_WARN("Quit since reply loop is stopped.");
  return false;
}

void ReplyLoop::replyTask(Task *task) {
  auto ts1InNano = utils::TimeUtil::currentTimeInNanos();
  auto replyLatency = (ts1InNano - task->mContext->getCreatedTimeInNanos()) / 1000000.0;
  SPDLOG_INFO("debug: reply queue: {}", replyLatency);

  if (task->mIndex != 0) {
    /// write task, we wait for index
    bool isCommitted = waitTillCommittedOrQuit(task->mIndex, task->mTerm);
    if (!isCommitted) {
      task->mCode = proto::ResponseCode::NOT_LEADER;
      task->mMessage = "NotLeaderAnyMore";
    } else {
      mSuccessWriteCounter.increase();
    }
  } else {
    /// read task, we only wait for version
    auto curMaxVersion = mVersionStore->getCurMaxVersion();
    if (task->mVersion > curMaxVersion) {
      /// the target version is not committed, reschedule this task
      /// SPDLOG_INFO("debug: waiting version {} for read, cur max {}", task->mVersion, curMaxVersion);
      pushTask(task->mIndex, task->mTerm, task->mVersion, task->mContext);
      /// release task
      task->mFlag = 2;
      return;
    } else {
      mSuccessReadCounter.increase();
    }
  }

  auto ts2InNano = utils::TimeUtil::currentTimeInNanos();

  if (task->mContext) {
    /// calculate total latency
    task->mOverallLatency = gringofts::TimeUtil::currentTimeInNanos() - task->mContext->getCreatedTimeInNanos();
    SPDLOG_INFO("debug: whole: {}", task->mOverallLatency / 1000000.0);
    /// response
    task->mContext->fillResponseAndReply(task->mCode, task->mMessage.c_str(), mRaftImpl->getLeaderHint());
  }

  auto ts3InNano = utils::TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("received on <index,term>=<{},{}>, replyCode={}, replyMessage={}, "
              "waitTillCommit cost {}ms, reply cost {}ms.",
              task->mIndex, task->mTerm, task->mCode, task->mMessage,
              (ts2InNano - ts1InNano) / 1000000.0,
              (ts3InNano - ts2InNano) / 1000000.0);

  /// update the global max version
  mVersionStore->updateCurMaxVersion(task->mVersion);
  /// release task
  task->mFlag = 2;
}

}  /// namespace goblin::kvengine::raft


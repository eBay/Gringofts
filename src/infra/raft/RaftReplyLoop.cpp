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

#include "RaftReplyLoop.h"
#include "../common_types.h"

namespace gringofts {
namespace raft {

RaftReplyLoop::RaftReplyLoop(const std::shared_ptr<RaftInterface> &raftImpl)
    : mRaftImpl(raftImpl) {
  mPopThread = std::thread(&RaftReplyLoop::popThreadMain, this);

  for (uint64_t i = 0; i < mConcurrency; ++i) {
    mReplyThreads.emplace_back(&RaftReplyLoop::replyThreadMain, this);
  }

  SPDLOG_INFO("ReplyLoop started, Concurrency={}.", mConcurrency);
}

RaftReplyLoop::~RaftReplyLoop() {
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

void RaftReplyLoop::pushTask(uint64_t index, uint64_t term,
                             RequestHandle *handle,
                             uint64_t code, const std::string &message) {
  auto taskPtr = std::make_shared<Task>();

  taskPtr->index = index;
  taskPtr->term = term;

  taskPtr->handle = handle;
  taskPtr->code = code;
  taskPtr->message = message;

  /// write lock
  std::unique_lock<std::shared_mutex> lock(mMutex);
  mTaskQueue.push_back(std::move(taskPtr));
}

void RaftReplyLoop::popThreadMain() {
  pthread_setname_np(pthread_self(), "ReplyLoop_pop");

  while (mRunning) {
    bool busy = false;

    {
      /// read lock
      std::shared_lock<std::shared_mutex> lock(mMutex);
      if (!mTaskQueue.empty() && mTaskQueue.front()->flag == 2) {
        busy = true;
      }
    }

    if (busy) {
      /// write lock
      std::unique_lock<std::shared_mutex> lock(mMutex);
      mTaskQueue.pop_front();
    } else {
      usleep(1000);   /// nothing to do, sleep 1ms
    }
  }
}

void RaftReplyLoop::replyThreadMain() {
  pthread_setname_np(pthread_self(), "ReplyLoop_reply");

  while (mRunning) {
    TaskPtr taskPtr;

    {
      /// read lock
      std::shared_lock<std::shared_mutex> lock(mMutex);

      for (auto &currPtr : mTaskQueue) {
        uint64_t expected = 0;
        if (currPtr->flag.compare_exchange_strong(expected, 1)) {
          taskPtr = currPtr;
          break;
        }
      }
    }

    if (taskPtr) {
      replyTask(taskPtr.get());
    } else {
      usleep(1000);   /// nothing to do, sleep 1ms
    }
  }
}

bool RaftReplyLoop::waitTillCommittedOrQuit(uint64_t index, uint64_t term) {
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

    LogEntry entry;
    assert(mRaftImpl->getEntry(index, &entry));
    return entry.term() == term;
  }

  SPDLOG_WARN("Quit since reply loop is stopped.");
  return false;
}

void RaftReplyLoop::replyTask(Task *task) {
  auto ts1InNano = TimeUtil::currentTimeInNanos();

  bool isCommitted = waitTillCommittedOrQuit(task->index, task->term);
  if (!isCommitted) {
    task->code = 301;
    task->message = "NotLeaderAnyMore";
  }

  auto ts2InNano = TimeUtil::currentTimeInNanos();

  if (task->handle) {
    task->handle->fillResultAndReply(task->code, task->message.c_str(), mRaftImpl->getLeaderHint());
  }

  auto ts3InNano = TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("received on <index,term>=<{},{}>, replyCode={}, replyMessage={}, "
              "waitTillCommit cost {}ms, reply cost {}ms.",
              task->index, task->term, task->code, task->message,
              (ts2InNano - ts1InNano) / 1000000.0,
              (ts3InNano - ts2InNano) / 1000000.0);

  /// release task
  task->flag = 2;
}

}  /// namespace raft
}  /// namespace gringofts

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

#ifndef SERVER_SRC_KV_ENGINE_RAFT_REPLYLOOP_H_
#define SERVER_SRC_KV_ENGINE_RAFT_REPLYLOOP_H_

#include <atomic>
#include <string>
#include <shared_mutex>

#include <infra/raft/RaftInterface.h>

#include "../model/Command.h"
#include "../store/VersionStore.h"
#include "../utils/Status.h"

namespace goblin::kvengine::raft {

class ReplyLoop {
 public:
  ReplyLoop(
      const std::shared_ptr<store::VersionStore> &versionStore,
      const std::shared_ptr<gringofts::raft::RaftInterface> &raftImpl);
  ReplyLoop(const ReplyLoop&) = delete;
  ReplyLoop &operator=(const ReplyLoop &) = delete;

  virtual ~ReplyLoop();

  /// send a task to reply loop
  void pushTask(
      uint64_t index,
      uint64_t term,
      const store::VersionType &version,
      std::shared_ptr<model::CommandContext> context);

 private:
  struct Task {
    /// <index, term> of log entry of raft
    uint64_t mIndex = 0;
    uint64_t mTerm  = 0;

    uint64_t mVersion = store::VersionStore::kInvalidVersion;

    /// context to reply
    std::shared_ptr<model::CommandContext> mContext = nullptr;

    /// code and msg to reply if <index, term> is committed
    proto::ResponseCode mCode = proto::ResponseCode::OK;
    std::string mMessage = "Success";

    /// 0:initial, 1:doing, 2:done
    std::atomic<uint64_t> mFlag = 0;
    gringofts::TimestampInNanos mOverallLatency = 0;
  };

  using TaskPtr = std::shared_ptr<Task>;

  /// thread function for ReplyLoop_pop
  void popThreadMain();

  /// thread function for ReplyLoop_reply
  void replyThreadMain();

  /// reply a task
  void replyTask(Task *task);

  /// wait for <index,term> to be committed or quit
  bool waitTillCommittedOrQuit(uint64_t index, uint64_t term);

  std::shared_ptr<store::VersionStore> mVersionStore;
  std::shared_ptr<const gringofts::raft::RaftInterface> mRaftImpl;

  /// task queue, push_back() and pop_front()
  std::list<TaskPtr> mTaskQueue;
  mutable std::shared_mutex mMutex;
  std::atomic<uint32_t> mTaskCount = 0;

  /**
   * threading model
   */
  std::atomic<bool> mRunning = true;

  /// num of concurrently running reply threads
  const uint64_t mConcurrency = 5;

  std::thread mPopThread;
  std::vector<std::thread> mReplyThreads;

  santiago::MetricsCenter::GaugeType mTaskCountGauge;
  santiago::MetricsCenter::CounterType mSuccessWriteCounter;
  santiago::MetricsCenter::CounterType mSuccessReadCounter;
};

}  /// namespace goblin::kvengine::raft

#endif  // SERVER_SRC_KV_ENGINE_RAFT_REPLYLOOP_H_


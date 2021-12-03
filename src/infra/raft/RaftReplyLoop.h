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

#ifndef SRC_INFRA_RAFT_RAFTREPLYLOOP_H_
#define SRC_INFRA_RAFT_RAFTREPLYLOOP_H_

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <thread>

#include <spdlog/spdlog.h>

#include "../grpc/RequestHandle.h"
#include "../util/TimeUtil.h"
#include "RaftInterface.h"

namespace gringofts {
namespace raft {

class RaftReplyLoop {
 public:
  explicit RaftReplyLoop(const std::shared_ptr<RaftInterface> &);

  /// forbidden copy/move
  RaftReplyLoop(const RaftReplyLoop&) = delete;
  RaftReplyLoop& operator=(const RaftReplyLoop&) = delete;

  ~RaftReplyLoop();

  /// send a task to reply loop
  void pushTask(uint64_t index, uint64_t term,
                RequestHandle *handle,
                uint64_t code, const std::string &message);

 private:
  struct Task {
    Task();
    ~Task();
    /// <index, term> of log entry of raft
    uint64_t index = 0;
    uint64_t term  = 0;

    /// handle to reply
    RequestHandle *handle = nullptr;

    /// code and msg to reply if <index, term> is committed
    uint64_t code = 200;
    std::string message = "Success";

    /// 0:initial, 1:doing, 2:done
    std::atomic<uint64_t> flag = 0;

    TimestampInNanos mTaskCreateTime = 0;
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

  /// raft
  std::shared_ptr<const RaftInterface> mRaftImpl;

  /// task queue, push_back() and pop_front()
  std::list<TaskPtr> mTaskQueue;
  std::atomic<uint64_t> mPendingReplyQueueSize = 0;
  mutable std::shared_mutex mMutex;

  /**
   * threading model
   */
  std::atomic<bool> mRunning = true;

  /// num of concurrently running reply threads
  const uint64_t mConcurrency = 2;

  std::thread mPopThread;
  std::vector<std::thread> mReplyThreads;

  santiago::MetricsCenter::GaugeType mPendingReplyGauge;
};

}  /// namespace raft
}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_RAFTREPLYLOOP_H_

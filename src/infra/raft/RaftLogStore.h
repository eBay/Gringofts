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

#ifndef SRC_INFRA_RAFT_RAFTLOGSTORE_H_
#define SRC_INFRA_RAFT_RAFTLOGSTORE_H_

#include "../es/Command.h"
#include "../es/Event.h"
#include "../grpc/RequestHandle.h"
#include "../util/CryptoUtil.h"
#include "../common_types.h"
#include "RaftInterface.h"

namespace gringofts {
namespace raft {

class RaftLogStore {
 public:
  RaftLogStore(const std::shared_ptr<RaftInterface> &raftImpl,
               const std::shared_ptr<CryptoUtil> &crypto);

  ~RaftLogStore();

  /**
   * CommandEventStore should call refresh() as many times as possible,
   * so that it won't miss term transition of raft.
   *
   * Attention: it is used by detectTransition() which only cares about
   *            leader or non-leader (both follower and candidate).
   */
  void refresh();

  /**
   * LogStoreTerm and LogStoreRole is changed by call of refresh()
   */
  uint64_t getLogStoreTerm() const { return mLogStoreTerm; }
  RaftRole getLogStoreRole() const { return mLogStoreRole; }

  void persistAsync(const std::shared_ptr<Command> &command,
                    const std::vector<std::shared_ptr<Event>> &events,
                    uint64_t index,
                    RequestHandle *requestHandle);

 private:
  /// thread function of persist thread
  void persistLoopMain();

  /// dequeue from input queue, insert into batch
  void dequeue();

  /// clear and send batch if batch size exceed or delay exceed
  void maySendBatch();

  /// log store state
  uint64_t mLogStoreTerm = 0;
  RaftRole mLogStoreRole = RaftRole::Follower;

  /// raft impl
  std::shared_ptr<RaftInterface> mRaftImpl;
  std::shared_ptr<CryptoUtil> mCrypto;

  /// input queue
  using PersistEntry = std::tuple<std::shared_ptr<Command>,
                                  std::vector<std::shared_ptr<Event>>,
                                  ClientRequest>;
  BlockingQueue<PersistEntry> mPersistQueue;

  /// persist thread
  std::thread mPersistLoop;
  std::atomic<bool> mRunning = true;

  uint64_t mLastSentTimeInNano = 0;
  ClientRequests mBatch;

  /// configurable vars
  const uint64_t kMaxDelayInMs = 20;
  const uint64_t kMaxBatchSize = 100;
  const uint64_t kMaxPayLoadSizeInBytes = 4000000;   /// less then 4M

  mutable santiago::MetricsCenter::GaugeType mGaugeRaftBatchSize;
};

}  /// namespace raft
}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_RAFTLOGSTORE_H_


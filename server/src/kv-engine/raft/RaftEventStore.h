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

#ifndef SERVER_SRC_KV_ENGINE_RAFT_RAFTEVENTSTORE_H_
#define SERVER_SRC_KV_ENGINE_RAFT_RAFTEVENTSTORE_H_

#include <shared_mutex>
#include <optional>

#include <infra/mpscqueue/MpscDoubleBufferQueue.h>
#include <infra/util/CryptoUtil.h>
#include <infra/raft/RaftInterface.h>

#include "../model/Event.h"
#include "../model/Command.h"
#include "ReplyLoop.h"

namespace goblin::kvengine::raft {

class RaftEventStore {
 public:
  template<class T> using BlockingQueue = gringofts::MpscDoubleBufferQueue<T>;
  using ClientRequestList = std::vector<gringofts::raft::ClientRequest>;
  using EventWithTermWithContext =
    std::tuple<model::EventList, uint64_t, store::VersionType, std::shared_ptr<model::CommandContext>>;
  using BundleWithIndex = std::pair<std::shared_ptr<proto::Bundle>, uint64_t>;

  RaftEventStore(
       std::shared_ptr<gringofts::raft::RaftInterface> raftImpl,
       std::shared_ptr<gringofts::CryptoUtil> cryptoUtil,
       std::shared_ptr<ReplyLoop> replyLoop);
  ~RaftEventStore();

  /// leadership detect
  utils::Status waitTillLeaderIsReadyOrStepDown(
       uint64_t expectedTerm,
       uint64_t *lastLeaderIndex,
       std::shared_ptr<proto::Bundle> *lastLeaderBundle,
       std::vector<gringofts::raft::MemberInfo> *clusterInfo);
  void updateLogIndexWhenNewLeader(uint64_t lastLogIndex);
  void getRoleAndTerm(gringofts::raft::RaftRole *curRole, uint64_t *curTerm);
  void refreshRoleAndTerm();
  std::optional<uint64_t> getLeaderHint();

  /// load events from raft
  void decryptEntries(std::vector<gringofts::raft::LogEntry> *entries, std::list<BundleWithIndex> *bundles);
  std::optional<BundleWithIndex> loadNextBundle();
  uint64_t loadBundles(uint64_t startIndex, uint64_t size, std::list<BundleWithIndex> *bundles);
  void resetLoadedLogIndex(uint64_t logIndex);

  /// persist events to raft
  void persistLoopMain();
  void persistAsync(
       uint64_t expectedTerm,
       std::shared_ptr<model::CommandContext> context,
       const store::VersionType &version,
       const model::EventList &events);
  void replyAsync(
       uint64_t expectedIndex,
       uint64_t expectedTerm,
       const store::VersionType &version,
       std::shared_ptr<model::CommandContext> context);
  void dequeue();
  void maySendBatch();

 private:
  std::shared_ptr<gringofts::CryptoUtil> mCrypto;
  std::shared_ptr<gringofts::raft::RaftInterface> mRaftImpl;
  std::shared_ptr<ReplyLoop> mReplyLoop;

  /**** leadership related ****/
  mutable std::shared_mutex mMutex;
  uint64_t mLogStoreTerm = 0;
  gringofts::raft::RaftRole mLogStoreRole = gringofts::raft::RaftRole::Follower;
  uint64_t mNextDetectLeadershipTimeInNano = 0;

  /**** fields related to loading from raft ****/
  /// cached bundles
  std::list<BundleWithIndex> mCachedBundles;
  std::atomic<uint64_t> mAppliedIndex = 0;
  std::atomic<uint64_t> mLoadedIndex = 0;

  /**** fields related to persisting to raft ****/
  /// persist thread
  std::thread mPersistLoop;
  std::atomic<bool> mRunning;

  BlockingQueue<EventWithTermWithContext> mPersistQueue;
  std::atomic<uint32_t> mTaskCount = 0;
  ClientRequestList mBatchReqs;
  uint64_t mLastSentTimeInNano = 0;
  std::atomic<uint64_t> mLastSentLogIndex = 0;

  /// configurable vars
  const uint64_t kDetectLeadershipTimeoutInNano = 1 * 1000 * 1000;  /// 1 millisecond
  const uint64_t kMaxDelayInMs = 20;
  const uint64_t kMaxBatchSize = 100;
  const uint64_t kMaxPayLoadSizeInBytes = 4000000;   /// less then 4M
  /**
    * optimize for sync load
    */
  const uint64_t kSpinLimit = 50;
  uint64_t mSpinTimes = 0;

  santiago::MetricsCenter::GaugeType mTaskCountGauge;
};
}  /// namespace goblin::kvengine::raft

#endif  // SERVER_SRC_KV_ENGINE_RAFT_RAFTEVENTSTORE_H_

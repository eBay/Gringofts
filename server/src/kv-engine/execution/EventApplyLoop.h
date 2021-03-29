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

#ifndef SERVER_SRC_KV_ENGINE_EXECUTION_EVENTAPPLYLOOP_H_
#define SERVER_SRC_KV_ENGINE_EXECUTION_EVENTAPPLYLOOP_H_

#include <infra/util/TestPointProcessor.h>

#include "../store/InMemoryKVStore.h"
#include "../store/RocksDBKVStore.h"
#include "../model/Event.h"
#include "../raft/RaftEventStore.h"

namespace goblin {
namespace mock {
template <typename ClusterType>
class MockAppCluster;
}
}

namespace goblin::kvengine::execution {

class EventApplyService {
 public:
  virtual ~EventApplyService() = default;
  virtual store::VersionType getLastAppliedVersion() const = 0;
};

class EventApplyLoop : public EventApplyService {
 public:
  EventApplyLoop(
      std::shared_ptr<raft::RaftEventStore> raftEventStore,
      std::shared_ptr<store::KVStore> kvStore) :
      mRaftEventStore(raftEventStore), mKVStore(kvStore),
      mLastMilestoneGauge(gringofts::getGauge("last_applied_index", {})) {
  }
  ~EventApplyLoop();
  void run();

  void shutdown() { mShouldExit = true; }

  store::VersionType getLastAppliedVersion() const override { return mLastMilestoneKeyVersion; }

  static constexpr uint32_t kApplyBatchSize = 10;
  static constexpr uint64_t kSaveMilestoneTimeoutInNano = (uint64_t)50 * 1000 * 1000;  /// 50ms

 protected:
  void recoverSelf();

  mutable gringofts::CryptoUtil mCrypto;

  std::atomic<bool> mShouldExit = false;

  mutable std::mutex mLoopMutex;

  /// should recover when started every time
  std::atomic<bool> mShouldRecover = true;

  uint64_t mLastMilestone = 0;
  gringofts::TimestampInNanos mLastSaveMilestoneTimeInNano = 0;
  std::atomic<store::VersionType> mLastMilestoneKeyVersion = 0;

  std::shared_ptr<store::KVStore> mKVStore;
  std::shared_ptr<raft::RaftEventStore> mRaftEventStore;

 private:
  EventApplyLoop(
      std::shared_ptr<raft::RaftEventStore> raftEventStore,
      std::shared_ptr<store::KVStore> kvStore,
      gringofts::TestPointProcessor *processor);
  gringofts::TestPointProcessor *mTPProcessor = nullptr;
  template <typename ClusterType>
  friend class mock::MockAppCluster;

  santiago::MetricsCenter::GaugeType mLastMilestoneGauge;
};
}  /// namespace goblin::kvengine::execution

#endif  // SERVER_SRC_KV_ENGINE_EXECUTION_EVENTAPPLYLOOP_H_

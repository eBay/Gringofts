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

#include "EventApplyLoop.h"

#include "../utils/TPRegistryEx.h"
#include "../utils/TimeUtil.h"

namespace goblin::kvengine::execution {

EventApplyLoop::~EventApplyLoop() {
  SPDLOG_INFO("deleting event apply loop");
}

void EventApplyLoop::run() {
  uint64_t appliedIndex = 0;
  std::shared_ptr<proto::Bundle> appliedBundle = nullptr;

  while (!mShouldExit) {
    std::lock_guard<std::mutex> lock(mLoopMutex);

    if (mShouldRecover) {
      recoverSelf();
    }

    auto tsInNano = utils::TimeUtil::currentTimeInNanos();

    if (appliedIndex > mLastMilestone &&
        (appliedIndex - mLastMilestone >= kApplyBatchSize ||
         tsInNano - mLastSaveMilestoneTimeInNano > kSaveMilestoneTimeoutInNano)) {
      auto s = mKVStore->commit(appliedIndex);
      if (!s.isOK()) {
        SPDLOG_ERROR("failed to commit milestone, milestone: {}, reason: {}", appliedIndex, s.getDetail());
        usleep(10 * 1000);  /// 10ms
        continue;
      }
      SPDLOG_INFO("commit milestone, newMilestone={}, oldMilestone={}, sinceLastTime={}us",
          appliedIndex,
          mLastMilestone,
          (tsInNano - mLastSaveMilestoneTimeInNano) / 1000.0);
      mLastMilestone = appliedIndex;
      mLastSaveMilestoneTimeInNano = tsInNano;
      mLastMilestoneGauge.set(appliedIndex);
      store::VersionType version;
      model::Event::getVersionFromBundle(*appliedBundle, &version);
      mLastMilestoneKeyVersion = version;
    }

    auto bundleWithIndexOpt = mRaftEventStore->loadNextBundle();
    if (!bundleWithIndexOpt) {
      continue;
    }

    while (!mShouldExit) {
      auto[bundle, index] = *bundleWithIndexOpt;

      model::EventList events;
      model::Event::fromBundle(*bundle, &events);
      utils::Status s = utils::Status::ok();
      for (auto &e : events) {
        /// TODO: support non-retryable operation like append
        s = e->apply(*mKVStore);
        if (!s.isOK()) {
          SPDLOG_ERROR("failed to apply event, index: {}, reason: {}", index, s.getDetail());
          break;
        }
      }
      TEST_POINT_WITH_TWO_ARGS(
          mTPProcessor,
          utils::TPRegistryEx::EventApplyLoop_run_interceptApplyResult,
          &s, &index);
      if (!s.isOK()) {
        SPDLOG_ERROR("failed to apply event, index: {}, reason: {}", index, s.getDetail());
        usleep(10 * 1000);  /// 10ms
        continue;
      }

      appliedIndex = index;
      appliedBundle = bundle;
      break;
    }
  }
}

void EventApplyLoop::recoverSelf() {
  store::MilestoneType milestone;
  auto s = mKVStore->loadMilestone(&milestone);
  if (s.isOK()) {
    SPDLOG_INFO("recover from milestone: {}", milestone);
    mRaftEventStore->resetLoadedLogIndex(milestone);
  } else if (s.isNotFound()) {
    SPDLOG_INFO("recover from milestone: {}", 0);
    mRaftEventStore->resetLoadedLogIndex(0);
  } else {
    assert(0);
  }
  mShouldRecover = false;
}

EventApplyLoop::EventApplyLoop(
    std::shared_ptr<raft::RaftEventStore> raftEventStore,
    std::shared_ptr<store::KVStore> kvStore,
    gringofts::TestPointProcessor *processor) :
    EventApplyLoop(raftEventStore, kvStore) {
  mTPProcessor = processor;
}
}  /// namespace goblin::kvengine::execution

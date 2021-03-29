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
#include "RaftEventStore.h"

#include <infra/common_types.h>

#include "../utils/TimeUtil.h"

namespace goblin::kvengine::raft {

RaftEventStore::RaftEventStore(
    std::shared_ptr<gringofts::raft::RaftInterface> raftImpl,
    std::shared_ptr<gringofts::CryptoUtil> cryptoUtil,
    std::shared_ptr<ReplyLoop> replyLoop):
    mRaftImpl(raftImpl),
    mCrypto(cryptoUtil),
    mReplyLoop(replyLoop),
    mTaskCountGauge(gringofts::getGauge("persist_queue_size", {})) {
  mRunning = true;
  mPersistLoop = std::thread(&RaftEventStore::persistLoopMain, this);
}

RaftEventStore::~RaftEventStore() {
  SPDLOG_INFO("deleting raft event store");
  mRunning = false;
  if (mPersistLoop.joinable()) {
    mPersistLoop.join();
  }
}

std::optional<RaftEventStore::BundleWithIndex> RaftEventStore::loadNextBundle() {
  if (!mCachedBundles.empty()) {
    auto bundleWithIndex = mCachedBundles.front();
    mCachedBundles.pop_front();
    /// update applied
    mAppliedIndex = std::max(mAppliedIndex.load(), bundleWithIndex.second);
    /// SPDLOG_INFO("debug: load index {}, apply index {}", mLoadedIndex, mAppliedIndex);
    return bundleWithIndex;
  }
  uint64_t commitIndex = mRaftImpl->getCommitIndex();
  ++mSpinTimes;

  /// mLoadedIndex is a persisted variable set by setCurrentOffset(),
  /// it may be legitimately greater than commitIndex.
  if (mLoadedIndex >= commitIndex) {
    if (mSpinTimes >= kSpinLimit) {
      usleep(1000);  /// 1ms
      mSpinTimes = 0;
    }
    return std::nullopt;
  }

  mSpinTimes = 0;

  std::list<BundleWithIndex> bundles;
  uint64_t size = loadBundles(mLoadedIndex + 1,
                              commitIndex - mLoadedIndex, &bundles);
  if (size == 0) {
    return std::nullopt;
  }

  mLoadedIndex += size;
  mCachedBundles.swap(bundles);
  return loadNextBundle();
}

uint64_t RaftEventStore::loadBundles(
    uint64_t startIndex,
    uint64_t size,
    std::list<BundleWithIndex> *bundles) {
  uint64_t ts1InNano = utils::TimeUtil::currentTimeInNanos();

  std::vector<gringofts::raft::LogEntry> entries;
  size = mRaftImpl->getEntries(startIndex, size, &entries);

  uint64_t ts2InNano = utils::TimeUtil::currentTimeInNanos();
  decryptEntries(&entries, bundles);

  uint64_t ts3InNano = utils::TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("EventStore Load {} Entry, totalCost={}ms, storageCost={}ms, decodeCost={}ms",
              size,
              (ts3InNano - ts1InNano) / 1000000.0,
              (ts2InNano - ts1InNano) / 1000000.0,
              (ts3InNano - ts2InNano) / 1000000.0);
  return size;
}

void RaftEventStore::resetLoadedLogIndex(uint64_t logIndex) {
  assert(mAppliedIndex == 0 && mLoadedIndex == 0);
  mAppliedIndex = logIndex;
  mLoadedIndex = logIndex;
}

void RaftEventStore::decryptEntries(std::vector<gringofts::raft::LogEntry> *entries,
                                                   std::list<BundleWithIndex> *bundles) {
  for (auto &entry : *entries) {
    if (entry.noop()) {
      continue;
    }

    if (mCrypto->isEnabled()) {
      if (entry.version().secret_key_version() == gringofts::SecretKey::kInvalidSecKeyVersion) {
        /// for compatibility: if no version field, use oldest version
        const auto &allVersions = mCrypto->getDescendingVersions();
        assert(!allVersions.empty());
        auto oldestVersion = allVersions.back();
        assert(mCrypto->decrypt(entry.mutable_payload(), oldestVersion) == 0);
      } else {
        assert(mCrypto->decrypt(entry.mutable_payload(), entry.version().secret_key_version()) == 0);
      }
    }

    auto bundlePtr = std::make_shared<proto::Bundle>();
    assert(bundlePtr->ParseFromString(entry.payload()));

    bundles->push_back({bundlePtr, entry.index()});
  }
}

utils::Status RaftEventStore::waitTillLeaderIsReadyOrStepDown(
    uint64_t expectedTerm,
    uint64_t *lastLeaderIndex,
    std::shared_ptr<proto::Bundle> *lastLeaderBundle,
    std::vector<gringofts::raft::MemberInfo> *clusterInfo) {
  SPDLOG_INFO("Start waiting for expected term {}. "
              "The hint noop is <index,term>=<{},{}>",
              expectedTerm, mRaftImpl->getLastLogIndex(), expectedTerm);

  while (1) {
    /// step 1
    *clusterInfo = mRaftImpl->getClusterMembers();
    auto currentTerm = mRaftImpl->getCurrentTerm();
    if (currentTerm != expectedTerm) {
      SPDLOG_WARN("Leader Step Down, since currentTerm {} != expectedTerm {}",
                  currentTerm, expectedTerm);
      *lastLeaderIndex = 0;
      *lastLeaderBundle = nullptr;
      return utils::Status::notLeader();
    }

    /// step 2
    uint64_t lastIndex = mRaftImpl->getLastLogIndex();
    uint64_t commitIndex = mRaftImpl->getCommitIndex();
    uint64_t loadedIndex = mLoadedIndex;
    uint64_t appliedIndex = mAppliedIndex;

    if (loadedIndex != commitIndex || commitIndex != lastIndex) {
      continue;
    }

    /// step 3 and step 4
    for (auto index = appliedIndex + 1; index <= lastIndex; ++index) {
      gringofts::raft::LogEntry entry;
      assert(mRaftImpl->getEntry(index, &entry));

      if (!entry.noop()) {
        break;
      }

      if (index == lastIndex && entry.term() == expectedTerm) {
        SPDLOG_INFO("Leader Is Ready for expected <index,term>=<{},{}>, appliedIndex={}",
                    lastIndex, expectedTerm, appliedIndex);
        std::list<BundleWithIndex> bundles;
        /// if appliedIndex is not zero, we should load last bundle
        if (mAppliedIndex > 0) {
          loadBundles(appliedIndex, 1, &bundles);
          assert(bundles.size() == 1);
          auto[bundle, index] = bundles.back();
          *lastLeaderBundle = bundle;
        } else {
          /// no bundles in the log
          *lastLeaderBundle = nullptr;
        }
        *lastLeaderIndex = lastIndex;
        return utils::Status::ok();
      }
    }
  }
}

void RaftEventStore::updateLogIndexWhenNewLeader(uint64_t lastLogIndex) {
  mLastSentLogIndex = lastLogIndex;
}

void RaftEventStore::getRoleAndTerm(gringofts::raft::RaftRole *curRole, uint64_t *curTerm) {
  std::shared_lock<std::shared_mutex> lock(mMutex);
  *curRole = mLogStoreRole;
  *curTerm = mLogStoreTerm;
}

void RaftEventStore::refreshRoleAndTerm() {
  uint64_t term1 = 0;
  uint64_t term2 = 1;
  gringofts::raft::RaftRole role;

  do {
    term1 = mRaftImpl->getCurrentTerm();
    role = mRaftImpl->getRaftRole();
    term2 = mRaftImpl->getCurrentTerm();
  } while (term1 != term2);

  if (term1 == mLogStoreTerm && role == mLogStoreRole) {
    return;
  }
  SPDLOG_INFO("found new role: {}, new term: {}", role, term1);
  std::unique_lock<std::shared_mutex> lock(mMutex);
  /// currentTerm is monotonically increased, so it is pretty sure
  /// that we've read a consistency pair of <currentTerm, raftRole>
  mLogStoreTerm = term1;
  mLogStoreRole = role;
}

std::optional<uint64_t> RaftEventStore::getLeaderHint() {
  return mRaftImpl->getLeaderHint();
}

void RaftEventStore::replyAsync(
    uint64_t expectedIndex,
    uint64_t expectedTerm,
    const store::VersionType &version,
    std::shared_ptr<model::CommandContext> context) {

  mReplyLoop->pushTask(expectedIndex, expectedTerm, version, context);
}

void RaftEventStore::persistAsync(
    uint64_t expectedTerm,
    std::shared_ptr<model::CommandContext> context,
    const store::VersionType &version,
    const model::EventList &events) {

  assert(!events.empty());
  /// SPDLOG_INFO("debug: persist queue {}", mTaskCount.load());
  mTaskCount += 1;
  mPersistQueue.enqueue({events, version, expectedTerm, context});
}

void RaftEventStore::dequeue() {
  /// if comes to here, all events will be nonRead events
  auto[events, version, expectedTerm, callData] = mPersistQueue.dequeue();

  uint64_t ts1InNano = utils::TimeUtil::currentTimeInNanos();

  uint64_t previousLogIndex = mLastSentLogIndex;
  auto nextLogIndex = previousLogIndex + 1;
  gringofts::raft::LogEntry entry;
  entry.mutable_version()->set_secret_key_version(mCrypto->getLatestSecKeyVersion());
  entry.set_term(expectedTerm);
  entry.set_index(nextLogIndex);
  entry.set_noop(false);
  gringofts::raft::ClientRequest req = {entry, nullptr};
  /// construct payload
  proto::Bundle bundle;
  assert(model::Event::toBundle(events, &bundle).isOK());
  bundle.SerializeToString(req.mEntry.mutable_payload());

  uint64_t ts2InNano = utils::TimeUtil::currentTimeInNanos();

  /// in-place encryption
  /// TODO: support encryption
  mCrypto->encrypt(req.mEntry.mutable_payload(),
      req.mEntry.version().secret_key_version());
  assert(req.mEntry.ByteSizeLong() <= kMaxPayLoadSizeInBytes);

  uint64_t ts3InNano = utils::TimeUtil::currentTimeInNanos();

  SPDLOG_INFO("Prepare Raft Log Entry, Id {}, applied {} events, encodeCost={}us, encryptCost={}us",
              nextLogIndex, events.size(),
              (ts2InNano - ts1InNano) / 1000.0,
              (ts3InNano - ts2InNano) / 1000.0);

  replyAsync(nextLogIndex, expectedTerm, version, callData);
  mBatchReqs.emplace_back(std::move(req));
  /// do CAS because it could be changed in another thread when a new leader comes into power
  mLastSentLogIndex.compare_exchange_strong(previousLogIndex, nextLogIndex);
}

void RaftEventStore::maySendBatch() {
  if (mBatchReqs.empty()) {
    return;
  }

  auto nowInNano = utils::TimeUtil::currentTimeInNanos();
  auto elapseInMs = static_cast<double>(nowInNano - mLastSentTimeInNano) / 1000000.0;

  if (mBatchReqs.size() < kMaxBatchSize && elapseInMs < kMaxDelayInMs) {
    return;
  }

  mTaskCountGauge.set(mTaskCount.load());

  SPDLOG_INFO("persistLoop send a batch, batchSize={}, elapseTime={}ms",
              mBatchReqs.size(), elapseInMs);

  mRaftImpl->enqueueClientRequests(std::move(mBatchReqs));
  mLastSentTimeInNano = nowInNano;
}

void RaftEventStore::persistLoopMain() {
  pthread_setname_np(pthread_self(), "RaftBatchThread");

  while (mRunning) {
    auto curTime = utils::TimeUtil::currentTimeInNanos();
    /// SPDLOG_INFO("debug: detect {} {}", mNextDetectLeadershipTimeInNano, curTime);
    if (mNextDetectLeadershipTimeInNano == 0 || mNextDetectLeadershipTimeInNano < curTime) {
      refreshRoleAndTerm();
      mNextDetectLeadershipTimeInNano = utils::TimeUtil::currentTimeInNanos() + kDetectLeadershipTimeoutInNano;
    }

    if (mPersistQueue.size() != 0) {
      /// dump consumer queue
      for (auto leftSize = mPersistQueue.size(); leftSize > 0; --leftSize) {
        dequeue();
        mTaskCount -= 1;
      }
    } else if (!mPersistQueue.empty()) {
      /// flip producer/consumer queue
      dequeue();
      mTaskCount -= 1;
      /// SPDLOG_INFO("debug: flip queue {}", mTaskCount.load());
      /// TODO: optimize mpsc queue
      for (auto leftSize = mPersistQueue.size(); leftSize > 0; --leftSize) {
        dequeue();
        mTaskCount -= 1;
      }
    } else {
      /// sleep 1ms
      usleep(1000);
    }

    maySendBatch();
  }
}

}  /// namespace goblin::kvengine::raft

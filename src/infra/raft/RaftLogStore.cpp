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

#include "RaftLogStore.h"

#include <unistd.h>

#include "../es/store/CommandEventEncodeWrapper.h"

namespace {
using ::gringofts::es::CommandEntry;
using ::gringofts::es::EventEntry;
using ::gringofts::es::RaftPayload;
}

namespace gringofts {
namespace raft {

RaftLogStore::RaftLogStore(const std::shared_ptr<RaftInterface> &raftImpl,
                           const std::shared_ptr<CryptoUtil> &crypto)
    : mRaftImpl(raftImpl), mCrypto(crypto) {
  mPersistLoop = std::thread(&RaftLogStore::persistLoopMain, this);
}

RaftLogStore::~RaftLogStore() {
  mRunning = false;
  if (mPersistLoop.joinable()) {
    mPersistLoop.join();
  }
}

void RaftLogStore::refresh() {
  uint64_t term1 = 0;
  uint64_t term2 = 1;
  RaftRole role;

  do {
    term1 = mRaftImpl->getCurrentTerm();
    role = mRaftImpl->getRaftRole();
    term2 = mRaftImpl->getCurrentTerm();
  } while (term1 != term2);

  /**
   * First thing first:
   * 1) it is safe that leader step down during its term.
   * 2) we promised app layer that: leader never step down during its term.
   *
   * When leader step down to follower of newTerm, it will:
   * 1) change role to RaftRole::Follower,
   * 2) update currentTerm to newTerm.
   *
   * This is a lock-free procedure, so RaftLogStore might detect
   * this transition: term is not changed, leader become non-leader.
   */
  if (mLogStoreTerm == term1 && mLogStoreRole == RaftRole::Leader
      && role != RaftRole::Leader) {
    SPDLOG_WARN("Term is not changed, Leader become non-Leader. "
                "Just skip this transition.");
    return;
  }

  /// currentTerm is monotonically increased, so it is pretty sure
  /// that we've read a consistency pair of <currentTerm, raftRole>
  mLogStoreTerm = term1;
  mLogStoreRole = role;
}

void RaftLogStore::persistAsync(const std::shared_ptr<Command> &command,
                                const std::vector<std::shared_ptr<Event>> &events,
                                uint64_t index,
                                RequestHandle *requestHandle) {
  raft::LogEntry entry;

  entry.set_term(mLogStoreTerm);
  entry.set_index(index);
  entry.set_noop(false);

  mPersistQueue.enqueue(PersistEntry{command, events, {entry, requestHandle}});
}

void RaftLogStore::dequeue() {
  auto[command, events, clientRequest] = mPersistQueue.dequeue();

  uint64_t ts1InNano = TimeUtil::currentTimeInNanos();

  /// construct payload
  RaftPayload payload;

  CommandEventEncodeWrapper::encodeCommand(*command, payload.mutable_command());
  for (const auto &event : events) {
    CommandEventEncodeWrapper::encodeEvent(*event, payload.add_events());
  }

  payload.SerializeToString(clientRequest.mEntry.mutable_payload());

  uint64_t ts2InNano = TimeUtil::currentTimeInNanos();

  /// in-place encryption
  mCrypto->encrypt(clientRequest.mEntry.mutable_payload());
  assert(clientRequest.mEntry.ByteSizeLong() <= kMaxPayLoadSizeInBytes);

  uint64_t ts3InNano = TimeUtil::currentTimeInNanos();

  SPDLOG_INFO("Prepare Raft Log Entry, Id {}, applied {} events, encodeCost={}us, encryptCost={}us",
              command->getId(), events.size(),
              (ts2InNano - ts1InNano) / 1000.0,
              (ts3InNano - ts2InNano) / 1000.0);

  mBatch.emplace_back(std::move(clientRequest));
}

void RaftLogStore::maySendBatch() {
  if (mBatch.empty()) {
    return;
  }

  auto nowInNano = TimeUtil::currentTimeInNanos();
  auto elapseInMs = (nowInNano - mLastSentTimeInNano) / 1000000.0;

  if (mBatch.size() < kMaxBatchSize && elapseInMs < kMaxDelayInMs) {
    return;
  }

  SPDLOG_INFO("persistLoop send a batch, batchSize={}, elapseTime={}ms",
              mBatch.size(), elapseInMs);

  mRaftImpl->enqueueClientRequests(std::move(mBatch));
  mLastSentTimeInNano = nowInNano;
}

void RaftLogStore::persistLoopMain() {
  pthread_setname_np(pthread_self(), "RaftBatchThread");

  while (mRunning) {
    if (mPersistQueue.size() != 0) {
      /// dump consumer queue
      for (auto leftSize = mPersistQueue.size(); leftSize > 0; --leftSize) {
        dequeue();
      }
    } else if (!mPersistQueue.empty()) {
      /// flip producer/consumer queue
      dequeue();
    } else {
      /// sleep 1ms
      usleep(1000);
    }

    maySendBatch();
  }
}

}  /// namespace raft
}  /// namespace gringofts

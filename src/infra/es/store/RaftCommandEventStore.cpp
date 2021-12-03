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

#include "RaftCommandEventStore.h"

namespace gringofts {

RaftCommandEventStore::RaftCommandEventStore(const std::shared_ptr<RaftInterface> &raftImpl,
                                             const std::shared_ptr<CryptoUtil> &crypto) :
    mRaftImpl(raftImpl),
    mCrypto(crypto) {
  mRaftReplyLoop = std::make_unique<RaftReplyLoop>(mRaftImpl);
  mRaftLogStore = std::make_unique<RaftLogStore>(mRaftImpl, mCrypto);

  /// should i check if raft server is up and running?
  mLastCheckedTerm = mRaftLogStore->getLogStoreTerm();
  mLastCheckedRole = mRaftLogStore->getLogStoreRole();
}

void RaftCommandEventStore::persistAsync(const std::shared_ptr<Command> &command,
                                         const std::vector<std::shared_ptr<Event>> &events,
                                         uint64_t code,
                                         const std::string &message) {
  /// note that command id has already been
  /// set early in upper-layer (command process loop)
  const auto commandId = command->getId();

  /// command with no events should skip persist.
  if (events.empty()) {
    mRaftReplyLoop->pushTask(commandId, mLastCheckedTerm,
                             command->getRequestHandle(), code, message);
    return;
  }

  Id nextEventId = 0;
  for (const auto &event : events) {
    event->setId(nextEventId);
    event->setCommandId(commandId);
    ++nextEventId;
  }

  mRaftReplyLoop->pushTask(commandId, mLastCheckedTerm,
                           command->getRequestHandle(), code, message);

  /// TODO: remove client reply code in raft v1 and v2
  mRaftLogStore->persistAsync(command, events, commandId, nullptr);
}

Transition RaftCommandEventStore::detectTransition() {
  mRaftLogStore->refresh();

  uint64_t currentTerm = mRaftLogStore->getLogStoreTerm();
  RaftRole currentRole = mRaftLogStore->getLogStoreRole();

  /**
   * detection logic
   * Scenario #1: LastCheckedTerm < CurrentTerm
   * LastCheckedRole        CurrentRole         TransitionResult            Action
   * --------------------------------------------------------------------------------------------------
   * Leader                 Leader              OldLeaderToNewLeader        Destruct->Construct->Accept
   *                        Candidate|Follower  LeaderToFollower            Destruct->Reject
   * --------------------------------------------------------------------------------------------------
   * Follower|Candidate     Leader              FollowerToLeader            Construct->Accept
   *                        Follower|Candidate  OldFollowerToNewFollower    Reject
   * --------------------------------------------------------------------------------------------------
   *
   * Scenario #2: LastCheckedTerm == CurrentTerm
   * LastCheckedRole        CurrentRole         TransitionResult            Action
   * --------------------------------------------------------------------------------------------------
   * Leader                 Leader              SameLeader                  Accept
   *                        Candidate|Follower  [Impossible]                N/A
   * --------------------------------------------------------------------------------------------------
   * Follower|Candidate     Leader              FollowerToLeader            Construct->Accept
   *                        Follower|Candidate  SameFollower                Reject
   * --------------------------------------------------------------------------------------------------
   */
  Transition transition;
  if (mLastCheckedTerm != currentTerm) {
    assert(mLastCheckedTerm < currentTerm);
    if (mLastCheckedRole == RaftRole::Leader) {
      if (currentRole == RaftRole::Leader)
        transition = Transition::OldLeaderToNewLeader;
      else
        transition = Transition::LeaderToFollower;
    } else {
      if (currentRole == RaftRole::Leader)
        transition = Transition::FollowerToLeader;
      else
        transition = Transition::OldFollowerToNewFollower;
    }
  } else {
    if (mLastCheckedRole == RaftRole::Leader) {
      assert(currentRole == RaftRole::Leader);
      transition = Transition::SameLeader;
    } else {
      if (currentRole == RaftRole::Leader)
        transition = Transition::FollowerToLeader;
      else
        transition = Transition::SameFollower;
    }
  }

  mLastCheckedTerm = currentTerm;
  mLastCheckedRole = currentRole;

  return transition;
}

std::optional<uint64_t> RaftCommandEventStore::getLeaderHint() const {
  return mRaftImpl->getLeaderHint();
}

}  /// namespace gringofts

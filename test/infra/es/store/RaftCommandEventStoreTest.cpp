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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../../../src/infra/es/store/ReadonlyRaftCommandEventStore.h"
#include "../../../../src/infra/es/store/RaftCommandEventStore.h"
#include "../../../../src/infra/raft/RaftBuilder.h"
#include "../Dummies.h"

namespace gringofts::test {

using ::testing::Return;

class RaftInterfaceMock : public RaftInterface {
 public:
  RaftInterfaceMock() = default;
  ~RaftInterfaceMock() = default;

  MOCK_CONST_METHOD0(getRaftRole, RaftRole());
  MOCK_CONST_METHOD0(getCommitIndex, uint64_t());
  MOCK_CONST_METHOD0(getCurrentTerm, uint64_t());
  MOCK_CONST_METHOD0(getFirstLogIndex, uint64_t());
  MOCK_CONST_METHOD0(getLastLogIndex, uint64_t());
  MOCK_CONST_METHOD0(getBeginLogIndex, uint64_t());
  MOCK_CONST_METHOD0(getLeaderHint, std::optional<uint64_t>());
  MOCK_CONST_METHOD0(getClusterMembers, std::vector<raft::MemberInfo>());
  MOCK_CONST_METHOD2(getInSyncFollowers, void(const int64_t &, std::vector<raft::MemberOffsetInfo> *));
  // @formatter:off
  MOCK_CONST_METHOD2(getEntry, bool(uint64_t, raft::LogEntry*));
  MOCK_CONST_METHOD3(getEntries, uint64_t(uint64_t, uint64_t, std::vector<raft::LogEntry>*));
  // @formatter:on
  MOCK_METHOD1(enqueueClientRequests, void(raft::ClientRequests));
  MOCK_METHOD1(truncatePrefix, void(uint64_t));
};

TEST(RaftCommandEventStoreSimpleTest, DetectTransitionTest) {
  /// init
  auto raftInterfaceMock = std::make_shared<RaftInterfaceMock>();
  RaftCommandEventStore commandEventStore{raftInterfaceMock, nullptr};

  /// behavior
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(1));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Leader));
  /// set lastCheckedTerm and lastCheckedRole
  commandEventStore.detectTransition();

  /// assert
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(2));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Leader));
  auto transition = commandEventStore.detectTransition();
  EXPECT_EQ(Transition::OldLeaderToNewLeader, transition);

  /// behavior
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(3));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Leader));
  /// set lastCheckedTerm and lastCheckedRole
  commandEventStore.detectTransition();

  /// assert
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(4));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Follower));
  transition = commandEventStore.detectTransition();
  EXPECT_EQ(Transition::LeaderToFollower, transition);

  /// behavior
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(5));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Candidate));
  /// set lastCheckedTerm and lastCheckedRole
  commandEventStore.detectTransition();

  /// assert
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(6));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Leader));
  transition = commandEventStore.detectTransition();
  EXPECT_EQ(Transition::FollowerToLeader, transition);

  /// behavior
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(7));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Follower));
  /// set lastCheckedTerm and lastCheckedRole
  commandEventStore.detectTransition();

  /// assert
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(8));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Candidate));
  transition = commandEventStore.detectTransition();
  EXPECT_EQ(Transition::OldFollowerToNewFollower, transition);

  /// behavior
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(9));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Leader));
  /// set lastCheckedTerm and lastCheckedRole
  commandEventStore.detectTransition();

  /// assert
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(9));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Leader));
  transition = commandEventStore.detectTransition();
  EXPECT_EQ(Transition::SameLeader, transition);

  /// behavior
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(10));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Follower));
  /// set lastCheckedTerm and lastCheckedRole
  commandEventStore.detectTransition();

  /// assert
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(10));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Leader));
  transition = commandEventStore.detectTransition();
  EXPECT_EQ(Transition::FollowerToLeader, transition);

  /// behavior
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(11));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Candidate));
  /// set lastCheckedTerm and lastCheckedRole
  commandEventStore.detectTransition();

  /// assert
  ON_CALL((*raftInterfaceMock), getCurrentTerm()).WillByDefault(Return(11));
  ON_CALL((*raftInterfaceMock), getRaftRole()).WillByDefault(Return(RaftRole::Follower));
  transition = commandEventStore.detectTransition();
  EXPECT_EQ(Transition::SameFollower, transition);
}

TEST(RaftCommandEventStoreSimpleTest, ExceptionTest) {
  /// init
  DummyCommandDecoder commandDecoder;
  DummyEventDecoder eventDecoder;
  RaftCommandEventStore commandEventStore{nullptr, nullptr};
  ReadonlyRaftCommandEventStore readonlyCommandEventStore{nullptr, nullptr, nullptr, nullptr, false};

  /// assert
  EXPECT_NO_THROW(commandEventStore.run());
  EXPECT_NO_THROW(commandEventStore.shutdown());
  EXPECT_THROW(readonlyCommandEventStore.loadNextCommand(commandDecoder), std::runtime_error);
  EXPECT_THROW(readonlyCommandEventStore.loadCommandAfter(0, commandDecoder), std::runtime_error);
}

class RaftCommandEventStoreTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    Util::executeCmd("rm -rf ../test/infra/es/store/node && mkdir ../test/infra/es/store/node");
    INIReader reader("../test/infra/es/store/config/aes.ini");
    mCrypto = std::make_shared<CryptoUtil>();
    mCrypto->init(reader);
    gringofts::NodeId nodeId = 1;
    gringofts::ClusterInfo::Node node;
    node.mNodeId = nodeId;
    node.mHostName = "0.0.0.0";
    node.mPortForRaft = 5253;
    gringofts::ClusterInfo clusterInfo;
    clusterInfo.addNode(node);
    mRaftImpl = raft::buildRaftImpl("../test/infra/es/store/config/raft.ini", nodeId, clusterInfo);
    mCommandEventStore = std::make_unique<RaftCommandEventStore>(mRaftImpl, mCrypto);
    mCommandDecoder = std::make_shared<DummyCommandDecoder>();
    mEventDecoder = std::make_shared<DummyEventDecoder>();
  }

  virtual void TearDown() {
    mReadonlyCommandEventStore->teardown();
  }

  std::unique_ptr<RaftCommandEventStore> mCommandEventStore;
  std::unique_ptr<ReadonlyRaftCommandEventStore> mReadonlyCommandEventStore;
  std::shared_ptr<CryptoUtil> mCrypto;
  std::shared_ptr<RaftInterface> mRaftImpl;
  std::shared_ptr<DummyCommandDecoder> mCommandDecoder;
  std::shared_ptr<DummyEventDecoder> mEventDecoder;
};

TEST_F(RaftCommandEventStoreTest, PersistAndLoadTest) {
  /// init
  mReadonlyCommandEventStore =
      std::make_unique<ReadonlyRaftCommandEventStore>(mRaftImpl, mCommandDecoder, mEventDecoder, mCrypto, false);
  mReadonlyCommandEventStore->init();

  auto command0 = DummyCommand::createDummyCommand();

  auto command1 = DummyCommand::createDummyCommand();
  auto events1 = DummyEvent::createDummyEvents(2);

  auto command2 = DummyCommand::createDummyCommand();
  auto events2 = DummyEvent::createDummyEvents(3);

  /// behavior
  auto persistThread = std::thread([this]() {
    this->mCommandEventStore->run();
  });

  /// wait till leader, otherwise persist will fail
  Transition transition;
  do {
    sleep(1);
    transition = mCommandEventStore->detectTransition();
  } while (transition == Transition::LeaderToFollower
      || transition == Transition::OldFollowerToNewFollower
      || transition == Transition::SameFollower);
  // sleep 1s so that noop entry will be persisted before waiting
  sleep(1);

  auto currentTerm = mCommandEventStore->getCurrentTerm();
  mReadonlyCommandEventStore->setCurrentOffset(0);
  auto commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandDecoder, *mEventDecoder);
  /// load noop to advance loadedIndex
  EXPECT_TRUE(!commandEventsOpt);
  auto commandId = mReadonlyCommandEventStore->waitTillLeaderIsReadyOrStepDown(currentTerm);

  auto command0Id = commandId;
  command0->setId(command0Id);
  auto command1Id = ++commandId;
  command1->setId(command1Id);
  auto command2Id = ++commandId;
  command2->setId(command2Id);

  EXPECT_NO_THROW(mCommandEventStore->persistAsync(std::move(command0), {}, 0, "test0"));
  EXPECT_NO_THROW(mCommandEventStore->persistAsync(std::move(command1), events1, 0, "test1"));
  EXPECT_NO_THROW(mCommandEventStore->persistAsync(std::move(command2), events2, 0, "test2"));

  while (!commandEventsOpt) {
    commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandDecoder, *mEventDecoder);
    sleep(1);
  }

  /// will not run
  EXPECT_NO_THROW(mCommandEventStore->run());

  mCommandEventStore->shutdown();
  if (persistThread.joinable()) {
    persistThread.join();
  }

  /// assert
  EXPECT_TRUE(commandEventsOpt);
  EXPECT_EQ(0, mReadonlyCommandEventStore->getCurrentOffset());
  /// load to update appliedIndex
  commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandDecoder, *mEventDecoder);
  EXPECT_EQ(command1Id, mReadonlyCommandEventStore->getCurrentOffset());
  EXPECT_TRUE(commandEventsOpt);

  auto &loadedCommand = (*commandEventsOpt).first;
  auto &loadedEvents = (*commandEventsOpt).second;

  EXPECT_EQ(command2Id, loadedCommand->getId());
  EXPECT_EQ(1, loadedCommand->getCreatorId());
  EXPECT_EQ(2, loadedCommand->getGroupId());
  EXPECT_EQ(3, loadedCommand->getGroupVersion());
  EXPECT_EQ("dummy tracking context", loadedCommand->getTrackingContext());
  EXPECT_EQ("DummyCommand", loadedCommand->encodeToString());
  EXPECT_EQ(nullptr, loadedCommand->getRequestHandle());

  EXPECT_EQ(3, loadedEvents.size());
  auto index = 0;
  for (const auto &eventPtr : loadedEvents) {
    EXPECT_EQ(index, eventPtr->getId());
    EXPECT_EQ(command2Id, eventPtr->getCommandId());
    EXPECT_EQ(1, eventPtr->getCreatorId());
    EXPECT_EQ(2, eventPtr->getGroupId());
    EXPECT_EQ(3, eventPtr->getGroupVersion());
    EXPECT_EQ("DummyEvent", eventPtr->encodeToString());
    EXPECT_EQ("dummy tracking context", eventPtr->getTrackingContext());
    index += 1;
  }

  EXPECT_NO_THROW(mReadonlyCommandEventStore->truncatePrefix(commandId));
}

TEST_F(RaftCommandEventStoreTest, PersistAndAsyncLoadTest) {
  /// init
  mReadonlyCommandEventStore =
      std::make_unique<ReadonlyRaftCommandEventStore>(mRaftImpl, mCommandDecoder, mEventDecoder, mCrypto, true);
  mReadonlyCommandEventStore->init();

  auto command0 = DummyCommand::createDummyCommand();

  auto command1 = DummyCommand::createDummyCommand();
  auto events1 = DummyEvent::createDummyEvents(2);

  auto command2 = DummyCommand::createDummyCommand();
  auto events2 = DummyEvent::createDummyEvents(3);

  /// behavior
  auto persistThread = std::thread([this]() {
    this->mCommandEventStore->run();
  });

  /// wait till leader, otherwise persist will fail
  Transition transition;
  do {
    sleep(1);
    transition = mCommandEventStore->detectTransition();
  } while (transition == Transition::LeaderToFollower
      || transition == Transition::OldFollowerToNewFollower
      || transition == Transition::SameFollower);
  // sleep 1s so that noop entry will be persisted before waiting
  sleep(1);

  auto currentTerm = mCommandEventStore->getCurrentTerm();
  auto commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandDecoder, *mEventDecoder);
  /// load noop to advance loadedIndex
  EXPECT_TRUE(!commandEventsOpt);
  auto commandId = mReadonlyCommandEventStore->waitTillLeaderIsReadyOrStepDown(currentTerm);

  auto command0Id = commandId;
  command0->setId(command0Id);
  auto command1Id = ++commandId;
  command1->setId(command1Id);
  auto command2Id = ++commandId;
  command2->setId(command2Id);

  EXPECT_NO_THROW(mCommandEventStore->persistAsync(std::move(command0), {}, 0, "test0"));
  EXPECT_NO_THROW(mCommandEventStore->persistAsync(std::move(command1), events1, 0, "test1"));
  EXPECT_NO_THROW(mCommandEventStore->persistAsync(std::move(command2), events2, 0, "test2"));

  while (!commandEventsOpt) {
    commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandDecoder, *mEventDecoder);
    sleep(1);
  }

  /// will not run
  EXPECT_NO_THROW(mCommandEventStore->run());

  mCommandEventStore->shutdown();
  if (persistThread.joinable()) {
    persistThread.join();
  }

  /// assert
  EXPECT_TRUE(commandEventsOpt);
  EXPECT_EQ(0, mReadonlyCommandEventStore->getCurrentOffset());
  /// load to update appliedIndex
  commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandDecoder, *mEventDecoder);
  EXPECT_EQ(command1Id, mReadonlyCommandEventStore->getCurrentOffset());
  EXPECT_TRUE(commandEventsOpt);

  auto &loadedCommand = (*commandEventsOpt).first;
  auto &loadedEvents = (*commandEventsOpt).second;

  EXPECT_EQ(command2Id, loadedCommand->getId());
  EXPECT_EQ(1, loadedCommand->getCreatorId());
  EXPECT_EQ(2, loadedCommand->getGroupId());
  EXPECT_EQ(3, loadedCommand->getGroupVersion());
  EXPECT_EQ("dummy tracking context", loadedCommand->getTrackingContext());
  EXPECT_EQ("DummyCommand", loadedCommand->encodeToString());
  EXPECT_EQ(nullptr, loadedCommand->getRequestHandle());

  EXPECT_EQ(3, loadedEvents.size());
  auto index = 0;
  for (const auto &eventPtr : loadedEvents) {
    EXPECT_EQ(index, eventPtr->getId());
    EXPECT_EQ(command2Id, eventPtr->getCommandId());
    EXPECT_EQ(1, eventPtr->getCreatorId());
    EXPECT_EQ(2, eventPtr->getGroupId());
    EXPECT_EQ(3, eventPtr->getGroupVersion());
    EXPECT_EQ("DummyEvent", eventPtr->encodeToString());
    EXPECT_EQ("dummy tracking context", eventPtr->getTrackingContext());
    index += 1;
  }

  EXPECT_NO_THROW(mReadonlyCommandEventStore->truncatePrefix(commandId));
}

TEST_F(RaftCommandEventStoreTest, LoadNextEventTest) {
  /// init
  mReadonlyCommandEventStore =
      std::make_unique<ReadonlyRaftCommandEventStore>(mRaftImpl, mCommandDecoder, mEventDecoder, mCrypto, false);
  mReadonlyCommandEventStore->init();

  auto command0 = DummyCommand::createDummyCommand();

  auto command1 = DummyCommand::createDummyCommand();
  auto events1 = DummyEvent::createDummyEvents(2);

  auto command2 = DummyCommand::createDummyCommand();
  auto events2 = DummyEvent::createDummyEvents(3);

  /// behavior
  auto persistThread = std::thread([this]() {
    this->mCommandEventStore->run();
  });

  /// wait till leader, otherwise persist will fail
  Transition transition;
  do {
    sleep(1);
    transition = mCommandEventStore->detectTransition();
  } while (transition == Transition::LeaderToFollower
      || transition == Transition::OldFollowerToNewFollower
      || transition == Transition::SameFollower);
  // sleep 1s so that noop entry will be persisted before waiting
  sleep(1);

  auto currentTerm = mCommandEventStore->getCurrentTerm();
  auto commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandDecoder, *mEventDecoder);
  /// load noop to advance loadedIndex
  EXPECT_TRUE(!commandEventsOpt);
  auto commandId = mReadonlyCommandEventStore->waitTillLeaderIsReadyOrStepDown(currentTerm);

  auto command0Id = commandId;
  command0->setId(command0Id);
  auto command1Id = ++commandId;
  command1->setId(command1Id);
  auto command2Id = ++commandId;
  command2->setId(command2Id);

  EXPECT_NO_THROW(mCommandEventStore->persistAsync(std::move(command0), {}, 0, "test0"));
  EXPECT_NO_THROW(mCommandEventStore->persistAsync(std::move(command1), events1, 0, "test1"));
  EXPECT_NO_THROW(mCommandEventStore->persistAsync(std::move(command2), events2, 0, "test2"));

  while (!commandEventsOpt) {
    commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandDecoder, *mEventDecoder);
    sleep(1);
  }

  /// will not run
  EXPECT_NO_THROW(mCommandEventStore->run());

  mCommandEventStore->shutdown();
  if (persistThread.joinable()) {
    persistThread.join();
  }

  /// behavior
  auto nextEventPtr = mReadonlyCommandEventStore->loadNextEvent(*mEventDecoder);

  /// assert
  EXPECT_EQ(0, nextEventPtr->getId());
  EXPECT_EQ(command2Id, nextEventPtr->getCommandId());
  EXPECT_EQ(1, nextEventPtr->getCreatorId());
  EXPECT_EQ(2, nextEventPtr->getGroupId());
  EXPECT_EQ(3, nextEventPtr->getGroupVersion());
  EXPECT_EQ("DummyEvent", nextEventPtr->encodeToString());
  EXPECT_EQ("dummy tracking context", nextEventPtr->getTrackingContext());
}

TEST_F(RaftCommandEventStoreTest, LoadCommandEventsListTest) {
  /// init
  mReadonlyCommandEventStore =
      std::make_unique<ReadonlyRaftCommandEventStore>(mRaftImpl, mCommandDecoder, mEventDecoder, mCrypto, false);
  mReadonlyCommandEventStore->init();

  auto command0 = DummyCommand::createDummyCommand();

  auto command1 = DummyCommand::createDummyCommand();
  auto events1 = DummyEvent::createDummyEvents(2);

  auto command2 = DummyCommand::createDummyCommand();
  auto events2 = DummyEvent::createDummyEvents(3);

  /// behavior
  auto persistThread = std::thread([this]() {
    this->mCommandEventStore->run();
  });

  /// wait till leader, otherwise persist will fail
  Transition transition;
  do {
    sleep(1);
    transition = mCommandEventStore->detectTransition();
  } while (transition == Transition::LeaderToFollower
      || transition == Transition::OldFollowerToNewFollower
      || transition == Transition::SameFollower);
  // sleep 1s so that noop entry will be persisted before waiting
  sleep(1);

  auto currentTerm = mCommandEventStore->getCurrentTerm();
  auto commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandDecoder, *mEventDecoder);
  /// load noop to advance loadedIndex
  EXPECT_TRUE(!commandEventsOpt);
  auto commandId = mReadonlyCommandEventStore->waitTillLeaderIsReadyOrStepDown(currentTerm);

  auto command0Id = commandId;
  command0->setId(command0Id);
  auto command1Id = ++commandId;
  command1->setId(command1Id);
  auto command2Id = ++commandId;
  command2->setId(command2Id);

  EXPECT_NO_THROW(mCommandEventStore->persistAsync(std::move(command0), {}, 0, "test0"));
  EXPECT_NO_THROW(mCommandEventStore->persistAsync(std::move(command1), events1, 0, "test1"));
  EXPECT_NO_THROW(mCommandEventStore->persistAsync(std::move(command2), events2, 0, "test2"));

  while (!commandEventsOpt) {
    commandEventsOpt = mReadonlyCommandEventStore->loadNextCommandEvents(*mCommandDecoder, *mEventDecoder);
    sleep(1);
  }

  /// will not run
  EXPECT_NO_THROW(mCommandEventStore->run());

  mCommandEventStore->shutdown();
  if (persistThread.joinable()) {
    persistThread.join();
  }

  /// behavior
  ReadonlyCommandEventStore::CommandEventsList commandEventsList;
  auto ret = mReadonlyCommandEventStore->loadCommandEventsList(*mCommandDecoder,
                                                               *mEventDecoder,
                                                               command1Id,
                                                               3,
                                                               &commandEventsList);

  /// assert
  EXPECT_EQ(2, ret);
  auto loadedCommandId = command1Id;
  for (const auto &commandEvents : commandEventsList) {
    auto &command = commandEvents.first;
    auto &events = commandEvents.second;

    EXPECT_EQ(loadedCommandId, command->getId());
    EXPECT_EQ(1, command->getCreatorId());
    EXPECT_EQ(2, command->getGroupId());
    EXPECT_EQ(3, command->getGroupVersion());
    EXPECT_EQ("dummy tracking context", command->getTrackingContext());
    EXPECT_EQ("DummyCommand", command->encodeToString());
    EXPECT_EQ(nullptr, command->getRequestHandle());

    if (loadedCommandId == command1Id) {
      EXPECT_EQ(2, events.size());
    } else {
      EXPECT_EQ(3, events.size());
    }
    auto index = 0;
    for (const auto &eventPtr : events) {
      EXPECT_EQ(index, eventPtr->getId());
      EXPECT_EQ(loadedCommandId, eventPtr->getCommandId());
      EXPECT_EQ(1, eventPtr->getCreatorId());
      EXPECT_EQ(2, eventPtr->getGroupId());
      EXPECT_EQ(3, eventPtr->getGroupVersion());
      EXPECT_EQ("DummyEvent", eventPtr->encodeToString());
      EXPECT_EQ("dummy tracking context", eventPtr->getTrackingContext());
      index += 1;
    }

    loadedCommandId += 1;
  }
}

}  /// namespace gringofts::test

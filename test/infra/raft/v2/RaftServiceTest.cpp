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

#include <gtest/gtest.h>
#include <grpcpp/impl/codegen/proto_utils.h>

#include "../../../../src/infra/raft/v2/RaftService.h"

namespace gringofts::raft::v2 {

class RaftServiceBufferTest : public ::testing::Test {
 protected:
  // Keep the gRPC runtime initialized even when only these buffer tests are selected.
  std::shared_ptr<grpc::Channel> mChannel =
      grpc::CreateChannel("127.0.0.1:0", grpc::InsecureChannelCredentials());
};

TEST_F(RaftServiceBufferTest, WireCompatibilityAndSliceOwnership) {
  AppendEntries::Request control;
  control.set_term(3);
  control.set_leader_id(1);
  control.set_prev_log_index(127);
  control.set_prev_log_term(2);
  control.set_commit_index(128);
  control.mutable_metrics()->set_entries_count(4);
  auto expected = control;
  std::vector<storage::SerializedLogEntry> entries;
  std::vector<std::weak_ptr<const char>> owners;
  for (size_t size : {0, 127, 128, 65536}) {
    auto *entry = expected.add_entries();
    entry->set_index(128 + entries.size());
    entry->set_term(3);
    entry->set_payload(std::string(size, '\xff'));
    entry->mutable_version()->set_secret_key_version(2);
    auto bytes = std::make_shared<std::string>(entry->SerializeAsString());
    // Unknown fields must survive raw forwarding too.
    bytes->append("\xa0\x06\x07", 3);
    ASSERT_TRUE(entry->ParseFromString(*bytes));
    entries.push_back({std::shared_ptr<const char>(bytes, bytes->data()), bytes->size()});
    owners.push_back(entries.back().mData);
  }

  auto buffer = buildAppendEntriesBuffer(control, entries);
  auto retained = buffer;
  entries.clear();
  buffer.Clear();
  for (const auto &owner : owners) {
    EXPECT_FALSE(owner.expired());
  }
  AppendEntries::Request parsed;
  ASSERT_TRUE(grpc::SerializationTraits<AppendEntries::Request>::Deserialize(&retained, &parsed).ok());
  EXPECT_EQ(parsed.SerializeAsString(), expected.SerializeAsString());
  for (const auto &owner : owners) {
    EXPECT_TRUE(owner.expired());
  }
}

TEST_F(RaftServiceBufferTest, Heartbeat) {
  AppendEntries::Request control;
  control.set_term(4);
  control.set_leader_id(2);
  control.set_prev_log_index(100);
  control.mutable_metrics()->set_entries_count(0);
  auto buffer = buildAppendEntriesBuffer(control, {});
  AppendEntries::Request parsed;
  ASSERT_TRUE(grpc::SerializationTraits<AppendEntries::Request>::Deserialize(&buffer, &parsed).ok());
  EXPECT_EQ(parsed.SerializeAsString(), control.SerializeAsString());
}

class RaftServiceTest : public ::testing::Test {
 protected:
  class Service final : public Raft::Service {
   public:
    grpc::Status AppendEntriesV2(grpc::ServerContext *, const AppendEntries::Request *request,
                                 AppendEntries::Response *response) override {
      std::lock_guard<std::mutex> lock(mMutex);
      mRequest = *request;
      response->set_term(request->term());
      response->set_success(true);
      response->set_match_index(request->prev_log_index() + request->entries_size());
      *response->mutable_metrics() = request->metrics();
      return grpc::Status::OK;
    }

    grpc::Status RequestVoteV2(grpc::ServerContext *, const RequestVote::Request *request,
                               RequestVote::Response *response) override {
      response->set_term(request->term());
      response->set_vote_granted(true);
      return grpc::Status::OK;
    }

    AppendEntries::Request request() {
      std::lock_guard<std::mutex> lock(mMutex);
      return mRequest;
    }

   private:
    std::mutex mMutex;
    AppendEntries::Request mRequest;
  };

  void SetUp() override {
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &mPort);
    builder.RegisterService(&mService);
    mServer = builder.BuildAndStart();
    ASSERT_NE(mServer, nullptr);
    ASSERT_GT(mPort, 0);
  }

  void TearDown() override {
    if (mServer) {
      mServer->Shutdown();
    }
  }

  Service mService;
  int mPort = 0;
  std::unique_ptr<grpc::Server> mServer;
  EventQueue mQueue;
};

TEST_F(RaftServiceTest, RawAppendEntriesToTypedServerAndTypedVote) {
  RaftClient client("127.0.0.1:" + std::to_string(mPort), std::nullopt,
                    std::make_shared<DNSResolver>(), 2, &mQueue, 1000, 1000, true);
  AppendEntries::Request control;
  control.set_term(3);
  control.set_leader_id(1);
  control.set_prev_log_index(8);
  control.set_prev_log_term(2);
  control.mutable_metrics()->set_entries_count(1);
  LogEntry entry;
  entry.set_term(3);
  entry.set_index(9);
  entry.set_payload(std::string(65536, '\0'));
  auto bytes = std::make_shared<std::string>(entry.SerializeAsString());
  client.appendEntries(control, {{std::shared_ptr<const char>(bytes, bytes->data()), bytes->size()}});
  auto event = mQueue.dequeue();
  ASSERT_EQ(event->mType, RaftEventBase::Type::AppendEntriesResponse);
  const auto &call = dynamic_cast<AppendEntriesResponseEvent &>(*event).mPayload;
  ASSERT_TRUE(call->mStatus.ok());
  EXPECT_TRUE(call->mResponse.success());
  EXPECT_EQ(call->mResponse.match_index(), 9);
  EXPECT_EQ(call->mResponse.metrics().entries_count(), 1);
  auto received = mService.request();
  ASSERT_EQ(received.entries_size(), 1);
  EXPECT_EQ(received.entries(0).SerializeAsString(), entry.SerializeAsString());
  EXPECT_EQ(received.prev_log_index(), control.prev_log_index());

  client.appendEntries(control, {});
  event = mQueue.dequeue();
  ASSERT_EQ(event->mType, RaftEventBase::Type::AppendEntriesResponse);
  EXPECT_TRUE(dynamic_cast<AppendEntriesResponseEvent &>(*event).mPayload->mStatus.ok());
  EXPECT_EQ(mService.request().entries_size(), 0);

  RequestVote::Request vote;
  vote.set_term(4);
  vote.set_candidate_id(1);
  client.requestVote(vote);
  event = mQueue.dequeue();
  ASSERT_EQ(event->mType, RaftEventBase::Type::RequestVoteResponse);
  const auto &voteCall = dynamic_cast<RequestVoteResponseEvent &>(*event).mPayload;
  ASSERT_TRUE(voteCall->mStatus.ok());
  EXPECT_TRUE(voteCall->mResponse.vote_granted());
}

TEST_F(RaftServiceTest, ShutdownReleasesPendingRawRequests) {
  std::weak_ptr<const char> owner;
  {
    RaftClient client("127.0.0.1:" + std::to_string(mPort), std::nullopt,
                      std::make_shared<DNSResolver>(), 2, &mQueue, 1000, 1000);
    AppendEntries::Request control;
    control.set_term(1);
    LogEntry entry;
    entry.set_index(1);
    entry.set_payload(std::string(65536, 'x'));
    auto bytes = std::make_shared<std::string>(entry.SerializeAsString());
    storage::SerializedLogEntry serialized{std::shared_ptr<const char>(bytes, bytes->data()), bytes->size()};
    owner = serialized.mData;
    client.appendEntries(control, {serialized});
  }
  while (!mQueue.empty()) {
    mQueue.dequeue();
  }
  EXPECT_TRUE(owner.expired());
}

}  // namespace gringofts::raft::v2

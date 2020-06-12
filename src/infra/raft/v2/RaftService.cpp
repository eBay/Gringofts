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

#include "RaftService.h"

#include "../../monitor/MonitorTypes.h"

namespace gringofts {
namespace raft {
namespace v2 {

//////////////////////////// RaftServer ////////////////////////////

RaftServer::RaftServer(const std::string &ipPort,
                       std::optional<TlsConf> tlsConfOpt,
                       EventQueue *aeRvQueue)
    : mIpPort(ipPort), mTlsConfOpt(std::move(tlsConfOpt)), mAeRvQueue(aeRvQueue) {
  mServerLoop = std::thread(&RaftServer::serverLoopMain, this);
}

RaftServer::~RaftServer() {
  mRunning = false;

  /// attention to the shutdown sequence.
  mServer->Shutdown();
  mCompletionQueue->Shutdown();

  /// join event loop.
  if (mServerLoop.joinable()) {
    mServerLoop.join();
  }

  /// drain completion queue.
  void *tag;
  bool ok;
  while (mCompletionQueue->Next(&tag, &ok)) { ; }
}

void RaftServer::serverLoopMain() {
  pthread_setname_np(pthread_self(), "RaftServer");

  grpc::ServerBuilder builder;
  builder.AddListeningPort(mIpPort, TlsUtil::buildServerCredentials(mTlsConfOpt));
  builder.RegisterService(&mService);

  mCompletionQueue = builder.AddCompletionQueue();
  mServer = builder.BuildAndStart();

  SPDLOG_INFO("RaftServer listening on {}", mIpPort);

  /// Spawn a new CallData instance to serve new clients.
  new RequestVoteCallData(&mService, mCompletionQueue.get(), mAeRvQueue);
  new AppendEntriesCallData(&mService, mCompletionQueue.get(), mAeRvQueue);

  void *tag;  /// uniquely identifies a request.
  bool ok;

  while (mCompletionQueue->Next(&tag, &ok)) {
    if (!mRunning) {
      SPDLOG_INFO("Server loop quit.");
      return;
    }

    auto *callData = static_cast<CallDataBase *>(tag);

    if (!ok) {
      SPDLOG_WARN("Cannot proceed as callData is no longer valid, "
                  "probably because client has cancelled the request.");
      callData->failOver();
    } else {
      callData->proceed();
    }
  }
}

//////////////////////////// RaftClient ////////////////////////////

RaftClient::RaftClient(const std::shared_ptr<grpc::Channel> &channel,
                       uint64_t peerId, EventQueue *aeRvQueue)
    : mStub(Raft::NewStub(channel)), mPeerId(peerId), mAeRvQueue(aeRvQueue) {
  /// start AE_resp/RV_resp receiving thread
  mClientLoop = std::thread(&RaftClient::clientLoopMain, this);
}

RaftClient::~RaftClient() {
  mRunning = false;

  /// shut down CQ
  mCompletionQueue.Shutdown();

  /// join event loop.
  if (mClientLoop.joinable()) {
    mClientLoop.join();
  }

  /// drain completion queue.
  void *tag;
  bool ok;
  while (mCompletionQueue.Next(&tag, &ok)) { ; }
}

void RaftClient::requestVote(const RequestVote::Request &request) {
  auto *call = new RequestVoteClientCall;

  call->mPeerId = mPeerId;

  std::chrono::time_point deadline = std::chrono::system_clock::now()
      + std::chrono::milliseconds(RaftConstants::RequestVote::kRpcTimeoutInMillis);
  call->mContext.set_deadline(deadline);

  call->mResponseReader = mStub->PrepareAsyncRequestVoteV2(&call->mContext, request, &mCompletionQueue);
  call->mResponseReader->StartCall();
  call->mResponseReader->Finish(&call->mResponse,
                                &call->mStatus,
                                reinterpret_cast<void *>(call));
}

void RaftClient::appendEntries(const AppendEntries::Request &request) {
  auto *call = new AppendEntriesClientCall;

  call->mPeerId = mPeerId;

  std::chrono::time_point deadline = std::chrono::system_clock::now()
      + std::chrono::milliseconds(RaftConstants::AppendEntries::kRpcTimeoutInMillis);
  call->mContext.set_deadline(deadline);

  call->mResponseReader = mStub->PrepareAsyncAppendEntriesV2(&call->mContext, request, &mCompletionQueue);
  call->mResponseReader->StartCall();
  call->mResponseReader->Finish(&call->mResponse,
                                &call->mStatus,
                                reinterpret_cast<void *>(call));
}

void RaftClient::clientLoopMain() {
  auto peerThreadName = std::string("RaftClient_") + std::to_string(mPeerId);
  pthread_setname_np(pthread_self(), peerThreadName.c_str());

  void *tag;  /// The tag is the memory location of the call object
  bool ok = false;

  /// Block until the next result is available in the completion queue.
  while (mCompletionQueue.Next(&tag, &ok)) {
    if (!mRunning) {
      SPDLOG_INFO("Client loop quit.");
      return;
    }

    auto *call = static_cast<AsyncClientCallBase *>(tag);
    GPR_ASSERT(ok);

    if (!call->mStatus.ok()) {
      SPDLOG_WARN("{} failed., gRpc error_code: {}, error_message: {}, error_details: {}",
                  call->toString(),
                  call->mStatus.error_code(),
                  call->mStatus.error_message(),
                  call->mStatus.error_details());

      /// collect gRpc error code metrics
      getCounter("grpc_error_counter", {{"error_code", std::to_string(call->mStatus.error_code())}});
    }

    /// enqueue event
    if (call->getType() == RaftEventBase::Type::RequestVoteResponse) {
      using EventType = RaftEvent<std::unique_ptr<RequestVoteClientCall>>;

      auto event = std::make_shared<EventType>();
      event->mType = RaftEventBase::Type::RequestVoteResponse;
      event->mPayload = std::unique_ptr<RequestVoteClientCall>(
          dynamic_cast<RequestVoteClientCall *>(call));
      mAeRvQueue->enqueue(event);
    } else if (call->getType() == RaftEventBase::Type::AppendEntriesResponse) {
      using EventType = RaftEvent<std::unique_ptr<AppendEntriesClientCall>>;

      auto event = std::make_shared<EventType>();
      event->mType = RaftEventBase::Type::AppendEntriesResponse;
      event->mPayload = std::unique_ptr<AppendEntriesClientCall>(
          dynamic_cast<AppendEntriesClientCall *>(call));

      (*event->mPayload->mResponse.mutable_metrics())
          .set_response_event_enqueue_time(TimeUtil::currentTimeInNanos());

      mAeRvQueue->enqueue(event);
    } else {
      SPDLOG_ERROR("RaftClient receive unknown event type: {}",
                   static_cast<uint64_t>(call->getType()));
      assert(0);
    }
  }
}

}  /// namespace v2
}  /// namespace raft
}  /// namespace gringofts

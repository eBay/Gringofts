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

#ifndef SRC_INFRA_RAFT_V2_RAFTSERVICE_H_
#define SRC_INFRA_RAFT_V2_RAFTSERVICE_H_

#include <memory>
#include <optional>
#include <shared_mutex>
#include <thread>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <spdlog/spdlog.h>

#include "../../grpc/RequestHandle.h"
#include "../../util/DNSResolver.h"
#include "../../util/TimeUtil.h"
#include "../../util/TlsUtil.h"
#include "../../common_types.h"
#include "../generated/raft.grpc.pb.h"
#include "../RaftConstants.h"
#include "../RaftInterface.h"

namespace gringofts {
namespace raft {
namespace v2 {

//////////////////////////// RaftEvent ////////////////////////////

struct RaftEventBase {
  /// dynamic_cast can only be used on Type with virtual Dtor.
  virtual ~RaftEventBase() = default;

  enum class Type {
    Unknown = 0,
    RequestVoteRequest = 1,
    RequestVoteResponse = 2,
    AppendEntriesRequest = 3,
    AppendEntriesResponse = 4,
    ClientRequest = 5
  };

  Type mType = Type::Unknown;
};

/**
 * RaftEvent is a generalized container for Payload,
 * mType indicates the specific PayloadType.
 */
template<typename PayloadType>
struct RaftEvent : public RaftEventBase {
  PayloadType mPayload;
};

using EventQueue = BlockingQueue<std::shared_ptr<RaftEventBase>>;

//////////////////////////// CallData ////////////////////////////

struct CallDataBase {
  CallDataBase(gringofts::raft::Raft::AsyncService *service,
               grpc::ServerCompletionQueue *completionQueue,
               EventQueue *aeRvQueue)
      : mService(service), mCompletionQueue(completionQueue), mAeRvQueue(aeRvQueue) {}

  virtual ~CallDataBase() = default;

  virtual void proceed() = 0;
  virtual void failOver() = 0;
  virtual void reply() = 0;

  gringofts::raft::Raft::AsyncService *mService;
  grpc::ServerCompletionQueue *mCompletionQueue;
  EventQueue *mAeRvQueue;
};

template<typename RequestType, typename ResponseType>
struct CallData : public CallDataBase {
  CallData(gringofts::raft::Raft::AsyncService *service,
           grpc::ServerCompletionQueue *completionQueue,
           EventQueue *aeRvQueue)
      : CallDataBase(service, completionQueue, aeRvQueue), mResponder(&mContext) {
    /// Attention, call virtual function in Ctor/Dtor is not recommended.
    /// However, we do not rely on polymorphism here.
    proceed();
  }

  void proceed() override { assert(0); }

  void reply() override {
    mCallStatus = CallStatus::FINISH;
    mResponder.Finish(mResponse, grpc::Status::OK, this);
  }

  void failOver() override {
    SPDLOG_WARN("Fail over for CallData");
    new CallData<RequestType, ResponseType>(mService, mCompletionQueue, mAeRvQueue);
    delete this;
  }

  RequestType mRequest;
  ResponseType mResponse;

  enum class CallStatus { CREATE, PROCESS, FINISH };
  CallStatus mCallStatus = CallStatus::CREATE;

  grpc::ServerContext mContext;
  grpc::ServerAsyncResponseWriter<ResponseType> mResponder;
};

template<>
inline
void CallData<gringofts::raft::AppendEntries::Request, gringofts::raft::AppendEntries::Response>::proceed() {
  if (mCallStatus == CallStatus::CREATE) {
    mCallStatus = CallStatus::PROCESS;
    mService->RequestAppendEntriesV2(&mContext, &mRequest, &mResponder,
                                     mCompletionQueue, mCompletionQueue, this);
  } else if (mCallStatus == CallStatus::PROCESS) {
    new CallData<gringofts::raft::AppendEntries::Request,
                 gringofts::raft::AppendEntries::Response>(mService, mCompletionQueue, mAeRvQueue);

    /// payload is a pointer, RaftEvent does not handle
    /// life cycle of CallData, since CallData will suicide itself
    using EventType = RaftEvent<CallData<gringofts::raft::AppendEntries::Request,
                                         gringofts::raft::AppendEntries::Response> *>;

    auto event = std::make_shared<EventType>();
    event->mType = RaftEventBase::Type::AppendEntriesRequest;
    event->mPayload = this;

    (*mRequest.mutable_metrics()).set_request_event_enqueue_time(TimeUtil::currentTimeInNanos());

    mAeRvQueue->enqueue(event);
  } else {
    GPR_ASSERT(mCallStatus == CallStatus::FINISH);
    delete this;
  }
}

template<>
inline
void CallData<gringofts::raft::RequestVote::Request, gringofts::raft::RequestVote::Response>::proceed() {
  if (mCallStatus == CallStatus::CREATE) {
    mCallStatus = CallStatus::PROCESS;
    mService->RequestRequestVoteV2(&mContext, &mRequest, &mResponder,
                                   mCompletionQueue, mCompletionQueue, this);
  } else if (mCallStatus == CallStatus::PROCESS) {
    new CallData<gringofts::raft::RequestVote::Request,
                 gringofts::raft::RequestVote::Response>(mService, mCompletionQueue, mAeRvQueue);

    /// payload is a pointer, RaftEvent does not handle
    /// life cycle of CallData, since CallData will suicide itself
    using EventType = RaftEvent<CallData<gringofts::raft::RequestVote::Request,
                                         gringofts::raft::RequestVote::Response> *>;

    auto event = std::make_shared<EventType>();
    event->mType = RaftEventBase::Type::RequestVoteRequest;
    event->mPayload = this;

    mAeRvQueue->enqueue(event);
  } else {
    GPR_ASSERT(mCallStatus == CallStatus::FINISH);
    delete this;
  }
}

using AppendEntriesCallData = CallData<gringofts::raft::AppendEntries::Request, gringofts::raft::AppendEntries::Response>;
using RequestVoteCallData = CallData<gringofts::raft::RequestVote::Request, gringofts::raft::RequestVote::Response>;

//////////////////////////// RaftServer ////////////////////////////

class RaftServer {
 public:
  RaftServer(const std::string &ipPort,
             std::optional<TlsConf> tlsConfOpt,
             EventQueue *aeRvQueue,
             std::shared_ptr<DNSResolver> dnsResolver);

  ~RaftServer();

 private:
  void serverLoopMain();

  std::string mIpPort;
  std::optional<TlsConf> mTlsConfOpt;

  std::unique_ptr<grpc::ServerCompletionQueue> mCompletionQueue;
  gringofts::raft::Raft::AsyncService mService;
  std::unique_ptr<grpc::Server> mServer;
  std::shared_ptr<DNSResolver> mDNSResolver;

  /// event queue
  EventQueue *mAeRvQueue;

  /// flag to notify event loop to quit
  std::atomic<bool> mRunning = true;
  std::thread mServerLoop;
};

//////////////////////////// AsyncClientCall ////////////////////////////

struct AsyncClientCallBase {
  virtual ~AsyncClientCallBase() = default;

  virtual std::string toString() const = 0;
  virtual RaftEventBase::Type getType() const = 0;

  grpc::ClientContext mContext;
  grpc::Status mStatus;
  uint64_t mPeerId = 0;
};

template<typename ResponseType>
struct AsyncClientCall : public AsyncClientCallBase {
  std::string toString() const override { assert(0); }
  RaftEventBase::Type getType() const override { assert(0); }

  ResponseType mResponse;
  std::unique_ptr<grpc::ClientAsyncResponseReader<ResponseType>> mResponseReader;
};

template<>
inline
std::string AsyncClientCall<gringofts::raft::AppendEntries::Response>::toString() const {
  return "Leader sending AE_req to Follower " + std::to_string(mPeerId);
}

template<>
inline
RaftEventBase::Type AsyncClientCall<gringofts::raft::AppendEntries::Response>::getType() const {
  return RaftEventBase::Type::AppendEntriesResponse;
}

template<>
inline
std::string AsyncClientCall<gringofts::raft::RequestVote::Response>::toString() const {
  return "Candidate sending RV_req to Follower " + std::to_string(mPeerId);
}

template<>
inline
RaftEventBase::Type AsyncClientCall<gringofts::raft::RequestVote::Response>::getType() const {
  return RaftEventBase::Type::RequestVoteResponse;
}

using AppendEntriesClientCall = AsyncClientCall<gringofts::raft::AppendEntries::Response>;
using RequestVoteClientCall = AsyncClientCall<gringofts::raft::RequestVote::Response>;

//////////////////////////// RaftClient ////////////////////////////

class RaftClient {
 public:
  RaftClient(const std::string &peerHostname,
             std::optional<TlsConf> tlsConfOpt,
             std::shared_ptr<DNSResolver> dnsResolver,
             uint64_t peerId,
             EventQueue *aeRvQueue);
  ~RaftClient();

  void requestVote(const gringofts::raft::RequestVote::Request &request);
  void appendEntries(const gringofts::raft::AppendEntries::Request &request);

 private:
  void refressChannel();
  /// thread function of mClientLoop.
  void clientLoopMain();

  std::string mPeerAddress;
  std::string mResolvedPeerAddress;
  std::optional<TlsConf> mTLSConfOpt;
  std::shared_ptr<DNSResolver> mDNSResolver;
  uint64_t mPeerId = 0;
  std::unique_ptr<gringofts::raft::Raft::Stub> mStub;
  std::shared_mutex mMutex;  /// the lock to guarantee thread-safe access of mStub
  grpc::CompletionQueue mCompletionQueue;

  /// event queue
  EventQueue *mAeRvQueue;

  /// flag that notify resp receive thread to quit
  std::atomic<bool> mRunning = true;
  std::thread mClientLoop;
};

//////////////////////////// Alias ////////////////////////////

using AppendEntriesRequestEvent = RaftEvent<AppendEntriesCallData *>;
using AppendEntriesResponseEvent = RaftEvent<std::unique_ptr<AppendEntriesClientCall>>;

using RequestVoteRequestEvent = RaftEvent<RequestVoteCallData *>;
using RequestVoteResponseEvent = RaftEvent<std::unique_ptr<RequestVoteClientCall>>;

using ClientRequestsEvent = RaftEvent<ClientRequests>;

}  /// namespace v2
}  /// namespace raft
}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_V2_RAFTSERVICE_H_

/************************************************************************
Copyright 2019-2021 eBay Inc.
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

#ifndef SRC_APP_UTIL_REQUESTCALLDATA_H_
#define SRC_APP_UTIL_REQUESTCALLDATA_H_

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include "AppInfo.h"
#include "generated/grpc/scale.grpc.pb.h"
#include "../infra/grpc/RequestHandle.h"
#include "../infra/es/Command.h"
#include "../infra/util/HttpCode.h"

namespace gringofts::app {

template<typename REQ>
struct BlackList {
  virtual ~BlackList() = default;
  virtual bool inBlackList(const REQ &) const = 0;
};

template<typename SVC, typename REQ, typename RES, typename CMD>
class CallDataHandler {
 public:
  typedef typename SVC::AsyncService AsyncService;
  typedef REQ Request;
  typedef RES Response;
  typedef CMD Command_t;
  typedef SVC Service;
  virtual grpc::Status buildResponse(const CMD &command,
                                     const std::vector<std::shared_ptr<Event>> &events,
                                     uint32_t code,
                                     const std::string &message,
                                     std::optional<uint64_t> leaderId,
                                     RES *response) = 0;

  virtual void request(typename SVC::AsyncService *service,
                       ::grpc::ServerContext *context,
                       REQ *request,
                       ::grpc::ServerAsyncResponseWriter<RES> *response,
                       ::grpc::ServerCompletionQueue *completionQueue,
                       void *tag) = 0;

  virtual std::shared_ptr<Command_t> buildCommand(
      const REQ &request, TimestampInNanos createdTime) = 0;
};

namespace detail {

// check if Request have req.header().namespace_()
template<class T, class = void>
struct has_header_namespace : std::false_type {};

template<class T>
struct has_header_namespace<T,
  std::void_t<decltype(std::declval<const T&>().header().namespace_())>
> : std::true_type {};

} // namespace detail

template<typename Handler>
class RequestCallData final : public RequestHandle {
 public:
  typedef typename Handler::AsyncService AsyncService;
  typedef typename Handler::Request Request;
  typedef typename Handler::Response Response;
  typedef typename Handler::Command_t Command_t;
  // Take in the "service" instance (in this case representing an asynchronous
  // server) and the completion queue "cq" used for asynchronous communication
  // with the gRPC runtime.
  RequestCallData(AsyncService *service, ::grpc::ServerCompletionQueue *cq,
                  BlockingQueue<std::shared_ptr<Command>>
                      &commandQueue,  // NOLINT(runtime/references)
                  const BlackList<Request> *blackList)
      : mService(service), mCompletionQueue(cq), mStatus(CREATE),
        mCommandQueue(commandQueue), mResponder(&mContext),
        mBlackList(blackList) {
    // Invoke the serving logic right away.
    proceed();
  }

  ~RequestCallData() override {
    reportLatency();
  }

  std::string getRequestNamespace() const override {
    using ReqT = std::decay_t<Request>;
    if constexpr (detail::has_header_namespace<ReqT>::value) {
      return std::string(mRequest.header().namespace_());
    } else {
      return {};
    }
  }

  void proceed() override {
    if (mStatus == CREATE) {
      // Make this instance progress to the PROCESS state.
      mStatus = PROCESS;

      // As part of the initial CREATE state, we *request* that the system
      // start processing SayHello requests. In this request, "this" acts as
      // the tag uniquely identifying the request (so that different CallData
      // instances can serve different requests concurrently), in this case
      // the memory address of this CallData instance.
      mHandler.request(mService, &mContext, &mRequest, &mResponder, mCompletionQueue, this);
    } else if (mStatus == PROCESS) {
      // Spawn a new CallData instance to serve new clients while we process
      // the one for this CallData. The instance will deallocate itself as
      // part of its FINISH state.
      new RequestCallData(mService, mCompletionQueue, mCommandQueue, mBlackList);

      if (!verifyClusterId()) {
        mStatus = FINISH;
        mResponder.Finish(mResponse, grpc::Status(grpc::INVALID_ARGUMENT, "forward to wrong cluster"), this);
        return;
      }

      // check black list
      if (mBlackList != nullptr && mBlackList->inBlackList(mRequest)) {
        fillResultAndReply(201, "Duplicated request", std::nullopt);
        return;
      }
      // build command
      mCommandCreateTime = TimeUtil::currentTimeInNanos();
      mCommand = mHandler.buildCommand(mRequest, mCommandCreateTime);
      mCommand->setRequestHandle(this);
      const std::string verifyResult = mCommand->verifyCommand();
      if (verifyResult != Command::kVerifiedSuccess) {
        SPDLOG_WARN("Request can not pass validation due to Error: {} Request: {}",
                    verifyResult, mRequest.DebugString());
        fillResultAndReply(HttpCode::BAD_REQUEST, verifyResult, std::nullopt);
        return;
      }
      // if the command is verified
      try {
        mCommandQueue.enqueue(mCommand);
      }
      catch (const QueueStoppedException &e) {
        SPDLOG_WARN(e.what());
        fillResultAndReply(HttpCode::SERVICE_UNAVAILABLE, std::string(e.what()), std::nullopt);
      }
    } else {
      GPR_ASSERT(mStatus == FINISH);
      // Once in the FINISH state, deallocate ourselves (CallData).
      delete this;
    }
  }

  void fillResultAndReply(uint32_t code,
                          const std::string &message,
                          std::optional<uint64_t> leaderId) override {
    // build response
    fillResultAndReply({}, code, message, leaderId);
  }

  void fillResultAndReply(const std::vector<std::shared_ptr<Event>> &events,
                          uint32_t code,
                          const std::string &message,
                          std::optional<uint64_t> leaderId) override {
    // build response with events
    auto s = mHandler.buildResponse(*mCommand, events, code, message, leaderId, &mResponse);
    mStatus = FINISH;
    mResponder.Finish(mResponse, s, this);
  }

  void forwardResponseReply(void *response) {
    mResponse = std::move(*static_cast<Response *>(response));
    mStatus = FINISH;
    getCounter("forward_succ_counter",
               {{"service", Handler::Service::service_full_name()}}).increase();
    mResponder.Finish(mResponse, grpc::Status::OK, this);
  }

  grpc::ServerContext *getContext() {
    return &mContext;
  }

  void failOver() override {
    SPDLOG_WARN("Cannot proceed as callData is no longer valid probably because client has cancelled the request.");
    new RequestCallData(mService, mCompletionQueue, mCommandQueue, mBlackList);
    delete this;
  }

  // check grpc meta data from client before processing the request
  bool verifyClusterId() {
    std::multimap<grpc::string_ref, grpc::string_ref> clientMeta = mContext.client_metadata();
    auto reqIter = clientMeta.find(kReqSource);
    if (reqIter != clientMeta.end() && (reqIter->second).compare(kForwardReqSource) == 0) {
      auto clusterIter = clientMeta.find(kClusterId);
      if (clusterIter != clientMeta.end()) {
        auto myClusterId = std::to_string(app::AppInfo::groupId());
        if ((clusterIter->second).compare(myClusterId.c_str()) != 0) {
          SPDLOG_WARN("req clusterId does not match this clusterid {},reject the request", myClusterId);
          return false;
        }
      }
    }
    return true;
  }

 protected:
  // The means of communication with the gRPC runtime for an asynchronous
  // server.
  AsyncService *mService;
  // The producer-consumer queue where for asynchronous server notifications.
  ::grpc::ServerCompletionQueue *mCompletionQueue;
  // Context for the rpc, allowing to tweak aspects of it such as the use
  // of compression, authentication, as well as to send metadata back to the
  // client.
  ::grpc::ServerContext mContext;

  // Let's implement a tiny state machine with the following states.
  enum CallStatus { CREATE, PROCESS, FINISH };
  CallStatus mStatus;  // The current serving state.

  BlockingQueue<std::shared_ptr<Command>> &mCommandQueue;

  // What we get from the client.
  Request mRequest;
  // What we send back to the client.
  Response mResponse;
  // Command built
  std::shared_ptr<Command_t> mCommand;

  // The means to get back to the client.
  ::grpc::ServerAsyncResponseWriter<Response> mResponder;

  const BlackList<Request> *mBlackList;
  // handler implementation
  Handler mHandler;
};

}  // namespace gringofts::app

#endif  // SRC_APP_UTIL_REQUESTCALLDATA_H_

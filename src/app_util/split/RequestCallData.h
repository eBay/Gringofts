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

#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_REQUESTCALLDATA_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_REQUESTCALLDATA_H_

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include "../generated/grpc/split.grpc.pb.h"
#include "../../infra/grpc/RequestHandle.h"
#include "../../infra/es/Command.h"

namespace gringofts::app::split {

class RequestCallData final : public RequestHandle {
 public:
  // Take in the "service" instance (in this case representing an asynchronous
  // server) and the completion queue "cq" used for asynchronous communication
  // with the gRPC runtime.
  RequestCallData(gringofts::app::split::SplitService::AsyncService *service,
                  ::grpc::ServerCompletionQueue *cq,
                  BlockingQueue<std::shared_ptr<Command>> &commandQueue);  // NOLINT(runtime/references)
  virtual ~RequestCallData() = default;

  void proceed() override;

  void fillResultAndReply(uint32_t code,
                          const std::string &message,
                          std::optional<uint64_t> leaderId) override;

 protected:
  // The means of communication with the gRPC runtime for an asynchronous
  // server.
  gringofts::app::split::SplitService::AsyncService *mService;
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
  gringofts::app::split::SplitRequest mRequest;
  // What we send back to the client.
  gringofts::app::split::SplitResponse mResponse;

  // The means to get back to the client.
  ::grpc::ServerAsyncResponseWriter<gringofts::app::split::SplitResponse> mResponder;
};

}  /// namespace gringofts::app

#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_REQUESTCALLDATA_H_

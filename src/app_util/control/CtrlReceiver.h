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

#ifndef SRC_APP_UTIL_CONTROL_CTRLRECEIVER_H_
#define SRC_APP_UTIL_CONTROL_CTRLRECEIVER_H_

#include "../MultiRequestReceiver.h"
#include "split/SplitCommand.h"
#include "reconfigure/ReconfigureCommand.h"
#include "../generated/grpc/ctrl.grpc.pb.h"

namespace gringofts::app::ctrl {

using protos::ScaleService;
using SplitRequest = protos::Scale::SplitRequest;
using SplitResponse = protos::Scale::SplitResponse;
using split::SplitCommand;
using ReconfigureRequest = protos::Reconfigure::Request;
using ReconfigureResponse = protos::Reconfigure::Response;
using reconfigure::ReconfigureCommand;

class SplitCallDataHandler : public CallDataHandler<ScaleService, SplitRequest, SplitResponse, SplitCommand> {
 public:
  grpc::Status buildResponse(const SplitCommand &command,
                             const std::vector<std::shared_ptr<Event>> &events,
                             uint32_t code,
                             const std::string &message,
                             std::optional<uint64_t> leaderId,
                             SplitResponse *response) override;

  void request(ScaleService::AsyncService *service,
               ::grpc::ServerContext *context,
               SplitRequest *request,
               ::grpc::ServerAsyncResponseWriter<SplitResponse> *responser,
               ::grpc::ServerCompletionQueue *completionQueue,
               void *tag) override;

  std::shared_ptr<SplitCommand> buildCommand(const SplitRequest &request, TimestampInNanos createdTime) override;
};

typedef RequestCallData<SplitCallDataHandler> SplitCallData;
class ReconfigureCallDataHandler :
  public CallDataHandler<ScaleService, ReconfigureRequest, ReconfigureResponse, ReconfigureCommand> {
 public:
  grpc::Status buildResponse(const ReconfigureCommand &command,
                             const std::vector<std::shared_ptr<gringofts::Event>> &events,
                             uint32_t code,
                             const std::string &message,
                             std::optional<uint64_t> leaderId,
                             ReconfigureResponse *response) override;

  void request(ScaleService::AsyncService *service,
               ::grpc::ServerContext *context,
               ReconfigureRequest *request,
               ::grpc::ServerAsyncResponseWriter<ReconfigureResponse> *responser,
               ::grpc::ServerCompletionQueue *completionQueue,
               void *tag) override;

  std::shared_ptr<ReconfigureCommand> buildCommand(const ReconfigureRequest &request,
                                                    TimestampInNanos createdTime) override;
};

typedef RequestCallData<ReconfigureCallDataHandler> ReconfigureCallData;

class CtrlServiceReceiver : public gringofts::app::MultiRequestReceiver<ScaleService::AsyncService> {
 public:
  explicit CtrlServiceReceiver(const INIReader &reader,
                                uint32_t port,
                                BlockingQueue<std::shared_ptr<Command>> &commandQueue,  // NOLINT(runtime/references)
                                int concurrency = 1) :
    MultiRequestReceiver(reader, port, commandQueue, "ctrlService", concurrency) {}

  void preSpawnCallData() override {
    int preSpawnCountInEachThread = 1;
    for (uint64_t i = 0; i < preSpawnCountInEachThread; ++i) {
      for (uint64_t j = 0; j < mConcurrency; ++j) {
        new SplitCallData(&mService, mCompletionQueues[j].get(), mCommandQueue, nullptr);
        new ReconfigureCallData(&mService, mCompletionQueues[j].get(), mCommandQueue, nullptr);
      }
    }
  }
};

}  // namespace gringofts::app::ctrl

#endif  // SRC_APP_UTIL_CONTROL_CTRLRECEIVER_H_

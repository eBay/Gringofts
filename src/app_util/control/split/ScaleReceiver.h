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

#ifndef SRC_APP_UTIL_CONTROL_SPLIT_SCALERECEIVER_H_
#define SRC_APP_UTIL_CONTROL_SPLIT_SCALERECEIVER_H_

#include "../../RequestReceiver.h"
#include "SplitCommand.h"

namespace gringofts::app::ctrl {

using protos::ScaleService;
using SplitRequest = protos::Scale::SplitRequest;
using SplitResponse = protos::Scale::SplitResponse;
using split::SplitCommand;

class SplitCallDataHandler : public CallDataHandler<ScaleService, SplitRequest, SplitResponse, SplitCommand> {
 public:
  grpc::Status buildResponse(const SplitCommand &command, uint32_t code,
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
typedef RequestReceiver<SplitCallData> ScaleServiceReceiver;

}  // namespace gringofts::app::ctrl

#endif  // SRC_APP_UTIL_CONTROL_SPLIT_SCALERECEIVER_H_

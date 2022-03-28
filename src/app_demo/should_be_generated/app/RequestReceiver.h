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
#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_REQUESTRECEIVER_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_REQUESTRECEIVER_H_

#include <INIReader.h>
#include <grpcpp/grpcpp.h>

#include  "../../../app_util/AppInfo.h"
#include  "../../../app_util/RequestCallData.h"
#include  "../../../app_util/RequestReceiver.h"
#include "../../../infra/es/Command.h"
#include "../../../infra/util/TlsUtil.h"
#include "../../generated/grpc/demo.grpc.pb.h"
#include "../domain/common_types.h"
#include "../domain/IncreaseCommand.h"

namespace gringofts::demo {
using protos::IncreaseRequest;
using protos::IncreaseResponse;
using protos::DemoService;

class CallDataHandler : public app::CallDataHandler<DemoService,
                                                    IncreaseRequest,
                                                    IncreaseResponse,
                                                    IncreaseCommand> {
 public:
  grpc::Status buildResponse(const IncreaseCommand &command, uint32_t code,
                             const std::string &message,
                             std::optional<uint64_t> leaderId,
                             IncreaseResponse *response) override;

  void request(DemoService::AsyncService *service,
               ::grpc::ServerContext *context,
               IncreaseRequest *request,
               ::grpc::ServerAsyncResponseWriter<IncreaseResponse> *responser,
               ::grpc::ServerCompletionQueue *completionQueue,
               void *tag) override;

  std::shared_ptr<IncreaseCommand> buildCommand(const IncreaseRequest &, TimestampInNanos) override;
};

typedef app::RequestCallData<CallDataHandler> CallData;
typedef app::RequestReceiver<CallData> RequestReceiver;

}  // namespace gringofts::demo
#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_REQUESTRECEIVER_H_

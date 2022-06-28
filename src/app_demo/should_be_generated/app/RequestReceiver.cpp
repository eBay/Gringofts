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

#include "RequestReceiver.h"

namespace gringofts::demo {

grpc::Status CallDataHandler::buildResponse(const IncreaseCommand &command, uint32_t code,
                                            const std::string &message,
                                            std::optional<uint64_t> leaderId,
                                            IncreaseResponse *response) {
  response->set_code(code);
  response->set_message(message);

  if (code == 301 && leaderId) {
    response->set_reserved(std::to_string(*leaderId));
  }

  if (!(code == 200 || code == 201 || code == 301 || code == 400 || code == 503)) {
    response->set_code(503);
  }

  return grpc::Status::OK;
}

void CallDataHandler::request(DemoService::AsyncService *service,
                              ::grpc::ServerContext *context,
                              IncreaseRequest *request,
                              ::grpc::ServerAsyncResponseWriter<IncreaseResponse> *responser,
                              ::grpc::ServerCompletionQueue *completionQueue,
                              void *tag) {
  service->RequestExecute(context, request, responser, completionQueue, completionQueue, tag);
}

std::shared_ptr<IncreaseCommand> CallDataHandler::buildCommand(
    const IncreaseRequest &request, TimestampInNanos createdTimeInNanos) {
  auto command = std::make_shared<IncreaseCommand>(createdTimeInNanos, request);
  command->setCreatorId(app::AppInfo::subsystemId());
  command->setGroupId(app::AppInfo::groupId());
  command->setGroupVersion(app::AppInfo::groupVersion());
  return command;
}

}  // namespace gringofts::demo

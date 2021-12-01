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

#include "ScaleReceiver.h"

namespace gringofts::app::ctrl {

using ResponseHeader = protos::Scale::ResponseHeader;

grpc::Status SplitCallDataHandler::buildResponse(const SplitCommand &command,
                                                 uint32_t code,
                                                 const std::string &message,
                                                 std::optional<uint64_t> leaderId,
                                                 SplitResponse *response) {
  ResponseHeader *header = response->mutable_header();
  header->set_code(code);
  response->set_tagcommitindex(command.getId());
  if (code == 301 && leaderId) {
    header->set_reserved(std::to_string(*leaderId));
  }
  header->set_message(message);
  return grpc::Status::OK;
}

void SplitCallDataHandler::request(ScaleService::AsyncService *service,
                                   ::grpc::ServerContext *context,
                                   SplitRequest *request,
                                   ::grpc::ServerAsyncResponseWriter<SplitResponse> *responser,
                                   ::grpc::ServerCompletionQueue *completionQueue,
                                   void *tag) {
  service->Requestscale(context, request, responser, completionQueue, completionQueue, tag);
}

std::shared_ptr<SplitCommand> SplitCallDataHandler::buildCommand(const SplitRequest &request,
                                                                 TimestampInNanos createdTime) {
  auto command = std::make_shared<SplitCommand>(createdTime, request);
  command->setCreatorId(app::AppInfo::subsystemId());
  command->setGroupId(app::AppInfo::groupId());
  command->setGroupVersion(app::AppInfo::groupVersion());
  return command;
}
}  // namespace gringofts::app::ctrl

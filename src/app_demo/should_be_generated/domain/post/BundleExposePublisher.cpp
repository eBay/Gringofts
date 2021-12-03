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

#include "BundleExposePublisher.h"

#include <spdlog/spdlog.h>

#include "../../../../app_util/AppInfo.h"
#include "../../../../app_util/CommandEventDecoderImpl.h"
#include "../CommandDecoderImpl.h"
#include "../EventDecoderImpl.h"
#include "../IncreaseCommand.h"

namespace gringofts {
namespace demo {

BundleExposePublisher::BundleExposePublisher(const INIReader &reader,
                                             std::unique_ptr<ReadonlyCommandEventStore> commandEventStore) :
    mReadonlyCommandEventStore(std::move(commandEventStore)) {
  mPort = gringofts::app::AppInfo::fetchPort();
  mMaxConcurrency = reader.GetInteger("publisher", "max.concurrency", -1);
  assert(mPort > 0 && mMaxConcurrency > 0);

  mTlsConfOpt = gringofts::TlsUtil::parseTlsConf(reader, "publisher.tls");
}

grpc::Status BundleExposePublisher::FetchBundle(grpc::ServerContext *context,
                                                const protos::FetchBundleRequest *request,
                                                protos::FetchBundleResponse *response) {
  SPDLOG_ERROR("FetchBundle CommandId:{} EventId:{}, Not implemented.",
               request->commandid(),
               request->starteventid());
  return ::grpc::Status::OK;
}

grpc::Status BundleExposePublisher::FetchBundles(grpc::ServerContext *context,
                                                 const protos::FetchBundlesRequest *request,
                                                 protos::FetchBundlesResponse *response) {
  uint64_t currentConcurrency = ++mCurrentConcurrency;
  grpc::Status status = grpc::Status::OK;

  if (currentConcurrency <= mMaxConcurrency) {
    status = fetchBundlesImpl(context, request, response);
  } else {
    response->mutable_status()->set_code(1);
    response->mutable_status()->set_errormessage("TooManyConnections");

    SPDLOG_WARN("trigger throttling, "
                "CurrentConcurrency={}, MaxConcurrency={}", currentConcurrency, mMaxConcurrency);
  }

  --mCurrentConcurrency;
  return status;
}

grpc::Status BundleExposePublisher::fetchBundlesImpl(grpc::ServerContext *context,
                                                     const protos::FetchBundlesRequest *request,
                                                     protos::FetchBundlesResponse *response) {
  app::CommandEventDecoderImpl<EventDecoderImpl, CommandDecoderImpl> decoder;
  Id startCommandId = request->startcommandid();
  if (startCommandId == 0) {
    startCommandId = 1;
  }
  Id endCommandId = request->endcommandid();
  ReadonlyCommandEventStore::CommandEventsList commandEventsList;
  auto size = mReadonlyCommandEventStore->loadCommandEventsList(decoder,
                                                                decoder,
                                                                startCommandId,
                                                                endCommandId - startCommandId + 1,
                                                                &commandEventsList);
  response->mutable_meta()->set_firstcommandid(startCommandId);
  if (size == 0) {
    response->mutable_meta()->set_lastcommandid(0);
  } else {
    response->mutable_meta()->set_lastcommandid(startCommandId + size - 1);
  }

  for (const auto &commandEvents : commandEventsList) {
    auto *bundle = response->add_bundleentries();
    // We don't need command payload right now.
    bundle->set_command_id(commandEvents.first->getId());
    bundle->set_value(
        dynamic_cast<IncreaseCommand *>(commandEvents.first.get())->getValue());
  }
  response->mutable_status()->set_code(0);
  response->mutable_status()->set_errormessage("Success");

  std::string condition = !request->has_condition() ? "All"
                                                    : request->condition().ismigrated() ? "isMigrated" : "Realtime";
  return ::grpc::Status::OK;
}

void BundleExposePublisher::run() {
  std::string server_address("0.0.0.0:" + std::to_string(mPort));
  grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, TlsUtil::buildServerCredentials(mTlsConfOpt));
  // Set send message limit to 40M to avoid message too big.
  builder.SetMaxSendMessageSize(40 << 20);
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(this);
  // Finally assemble the server.
  mServer = builder.BuildAndStart();
  SPDLOG_INFO("Server listening on {} ", server_address);

  mServer->Wait();
}

void BundleExposePublisher::shutdown() {
  mServer->Shutdown();
}

}  /// namespace demo
}  /// namespace gringofts

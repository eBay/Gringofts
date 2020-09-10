// THIS FILE IS AUTO-GENERATED, PLEASE DO NOT EDIT!!!
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

#include <spdlog/spdlog.h>

#include "RequestCallData.h"

namespace gringofts::app::split {

RequestReceiver::RequestReceiver(
    const INIReader &reader,
    BlockingQueue<std::shared_ptr<Command>> &commandQueue) :
    mCommandQueue(commandQueue) {
  mIpPort = reader.Get("receiver", "ip.port", "UNKNOWN");
  mIpPort = "0.0.0.0:61203";
  assert(mIpPort != "UNKNOWN");

  mTlsConfOpt = TlsUtil::parseTlsConf(reader, "tls");
}

void RequestReceiver::run() {
  if (mIsShutdown) {
    SPDLOG_WARN("Receiver is already down. Will not run again.");
    return;
  }

  std::string server_address(mIpPort);

  ::grpc::ServerBuilder builder;
  mCompletionQueue = builder.AddCompletionQueue();
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address,
                           TlsUtil::buildServerCredentials(mTlsConfOpt));
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&mService);


  mServer = builder.BuildAndStart();
  SPDLOG_INFO("Server listening on {}", server_address);

  // Spawn a new CallData instance to serve new clients.
  new RequestCallData(&mService, mCompletionQueue.get(), mCommandQueue);

  handleRpcs();

}

void RequestReceiver::handleRpcs() {
  void *tag;  // uniquely identifies a request.
  bool ok;
  // Block waiting to read the next event from the completion queue. The
  // event is uniquely identified by its tag, which in this case is the
  // memory address of a CallData instance.
  // The return value of Next should always be checked. This return value
  // tells us whether there is any kind of event or cq_ is shutting down.
  while (mCompletionQueue->Next(&tag, &ok)) {
    auto *callData = static_cast<RequestCallData *>(tag);
    if (ok) {
      callData->proceed();
    } else {
      SPDLOG_WARN("Cannot proceed as callData is no longer valid probably because client has cancelled the request.");
    }
  }
}

void RequestReceiver::shutdown() {
  if (mIsShutdown) {
    SPDLOG_INFO("Server is already down");
  } else {
    mIsShutdown = true;
    mServer->Shutdown();

    mCompletionQueue->Shutdown();
  }
}

}  /// namespace gringofts::app

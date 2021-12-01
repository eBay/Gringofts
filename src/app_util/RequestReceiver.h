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

#ifndef SRC_APP_UTIL_REQUESTRECEIVER_H_
#define SRC_APP_UTIL_REQUESTRECEIVER_H_

#include <INIReader.h>
#include <grpcpp/grpcpp.h>

#include "../infra/es/Command.h"
#include "../infra/util/TlsUtil.h"
#include "RequestCallData.h"
#include "Service.h"

using ::grpc::ServerCompletionQueue;

namespace gringofts::app {

template<typename CallData, int PreSpawn = 1, int Concurrency = 1>
class RequestReceiver final : public Service {
 public:
  typedef typename CallData::AsyncService AsyncService;
  typedef BlackList<typename CallData::Request> BlackList_t;
  // disallow copy ctor and copy assignment
  RequestReceiver(const RequestReceiver &) = delete;
  RequestReceiver &operator=(const RequestReceiver &) = delete;

  // disallow move ctor and move assignment
  RequestReceiver(RequestReceiver &&) = delete;
  RequestReceiver &operator=(RequestReceiver &&) = delete;

  ~RequestReceiver() override = default;

  explicit RequestReceiver(const INIReader &reader,
                           uint32_t port,
                           BlockingQueue<std::shared_ptr<Command>> &commandQueue)  // NOLINT(runtime/references)
      : RequestReceiver(reader, port, commandQueue, nullptr) {}

  explicit RequestReceiver(const INIReader &reader,
                           uint32_t port,
                           BlockingQueue<std::shared_ptr<Command>> &commandQueue,  // NOLINT(runtime/references)
                           std::unique_ptr<BlackList_t> blackList)
      : mCommandQueue(commandQueue), mBackList(std::move(blackList)) {
    mIpPort = "0.0.0.0:" + std::to_string(port);
    assert(mIpPort != "UNKNOWN");
    mTlsConfOpt = TlsUtil::parseTlsConf(reader, "tls");
  }

  void startListen() {
    if (mIsShutdown) {
      SPDLOG_WARN("Receiver is already down. Will not run again.");
      return;
    }

    std::string server_address(mIpPort);

    ::grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address,
                             TlsUtil::buildServerCredentials(mTlsConfOpt));
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&mService);

    for (uint64_t i = 0; i < Concurrency; ++i) {
      mCompletionQueues[i] = std::move(builder.AddCompletionQueue());
    }

    mServer = builder.BuildAndStart();
    SPDLOG_INFO("Server listening on {}", server_address);

    // Spawn a new CallData instance to serve new clients.
    for (uint64_t i = 0; i < PreSpawn; ++i) {
      for (uint64_t j = 0; j < Concurrency; ++j) {
        new CallData(&mService, mCompletionQueues[j].get(), mCommandQueue, mBackList.get());
      }
    }
  }

  void start() override {
    startListen();
    // start receive threads
    for (uint64_t i = 0; i < Concurrency; ++i) {
      mRcvThreads[i] = std::thread([this, i]() {
        std::string threadName = (std::string("RcvThread_") + std::to_string(i));
        pthread_setname_np(pthread_self(), threadName.c_str());
        handleRpcs(i);
      });
    }
  }

  void stop() override {
    if (mIsShutdown) {
      SPDLOG_INFO("Server is already down");
    } else {
      mIsShutdown = true;
      mServer->Shutdown();
      for (uint64_t i = 0; i < Concurrency; ++i) {
        mCompletionQueues[i]->Shutdown();
        /// drain completion queue.
        void *tag;
        bool ok;
        while (mCompletionQueues[i]->Next(&tag, &ok)) { ; }
      }
      // join threads
      for (uint64_t i = 0; i < Concurrency; ++i) {
        if (mRcvThreads[i].joinable()) {
          mRcvThreads[i].join();
        }
      }
    }
  }

 private:
  void handleRpcs(uint64_t i) {
    void *tag;  // uniquely identifies a request.
    bool ok;
    // Block waiting to read the next event from the completion queue. The
    // event is uniquely identified by its tag, which in this case is the
    // memory address of a CallData instance.
    // The return value of Next should always be checked. This return value
    // tells us whether there is any kind of event or cq_ is shutting down.
    while (mCompletionQueues[i]->Next(&tag, &ok)) {
      auto *callData = static_cast<RequestHandle *>(tag);
      if (ok) {
        callData->proceed();
      } else if (!mIsShutdown) {
        callData->failOver();
      }
    }
  }

  std::string mIpPort;
  std::optional<TlsConf> mTlsConfOpt;
  BlockingQueue<std::shared_ptr<Command>> &mCommandQueue;
  std::unique_ptr<::grpc::Server> mServer;
  std::atomic<bool> mIsShutdown = false;
  std::array<std::unique_ptr<ServerCompletionQueue>, Concurrency> mCompletionQueues;
  std::array<std::thread, Concurrency> mRcvThreads;
  AsyncService mService;
  std::unique_ptr<BlackList_t> mBackList;
};

}  /// namespace gringofts::app
#endif  // SRC_APP_UTIL_REQUESTRECEIVER_H_

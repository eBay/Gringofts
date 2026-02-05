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

#ifndef SRC_APP_UTIL_MULTIREQUESTRECEIVER_H_
#define SRC_APP_UTIL_MULTIREQUESTRECEIVER_H_

#include <INIReader.h>
#include <grpcpp/grpcpp.h>

#include "../infra/es/Command.h"
#include "../infra/util/TlsUtil.h"
#include "RequestCallData.h"
#include "Service.h"

using ::grpc::ServerCompletionQueue;

namespace gringofts::app {

template<typename AsyncService>
class MultiRequestReceiver : public Service {
 public:
  // disallow copy ctor and copy assignment
  MultiRequestReceiver(const MultiRequestReceiver &) = delete;
  MultiRequestReceiver &operator=(const MultiRequestReceiver &) = delete;

  // disallow move ctor and move assignment
  MultiRequestReceiver(MultiRequestReceiver &&) = delete;
  MultiRequestReceiver &operator=(MultiRequestReceiver &&) = delete;

  ~MultiRequestReceiver() override = default;

  explicit MultiRequestReceiver(const INIReader &reader,
                           uint32_t port,
                           BlockingQueue<std::shared_ptr<Command>> &commandQueue,  // NOLINT(runtime/references)
                           const std::string &receiverName,
                           int concurrency)
      : mReceiverName(receiverName), mCommandQueue(commandQueue), mConcurrency(concurrency),
        mCompletionQueues(mConcurrency), mRcvThreads(mConcurrency) {
    mIpPort = "0.0.0.0:" + std::to_string(port);
    mTlsConfOpt = TlsUtil::parseTlsConf(reader, "tls");
  }

  void startListen() {
    if (mIsShutdown) {
      SPDLOG_WARN("Receiver {} is already down. Will not run again.", mReceiverName);
      return;
    }

    std::string server_address(mIpPort);

    ::grpc::ServerBuilder builder;
    // Listen on the given address with authentication mechanism.
    builder.AddListeningPort(server_address,
                             TlsUtil::buildServerCredentials(mTlsConfOpt));
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&mService);

    assert(mConcurrency == mCompletionQueues.size());
    for (uint64_t i = 0; i < mConcurrency; ++i) {
      mCompletionQueues[i] = std::move(builder.AddCompletionQueue());
    }

    mServer = builder.BuildAndStart();
    SPDLOG_INFO("Server {} listening on {}", mReceiverName, server_address);
  }

  void start() override {
    startListen();
    preSpawnCallData();
    // start receive threads
    assert(mConcurrency == mRcvThreads.size());
    for (uint64_t i = 0; i < mConcurrency; ++i) {
      mRcvThreads[i] = std::thread([this, i]() {
        std::string threadName = (mReceiverName + "_" + std::to_string(i));
        pthread_setname_np(pthread_self(), threadName.c_str());
        handleRpcs(i);
      });
    }
  }

  void stop() override {
    if (mIsShutdown) {
      SPDLOG_INFO("Server {} is already down", mReceiverName);
    } else {
      mIsShutdown = true;
      mServer->Shutdown();
      assert(mConcurrency == mCompletionQueues.size());
      for (uint64_t i = 0; i < mConcurrency; ++i) {
        mCompletionQueues[i]->Shutdown();
        /// drain completion queue.
        void *tag;
        bool ok;
        while (mCompletionQueues[i]->Next(&tag, &ok)) { ; }
      }
      // join threads
      assert(mConcurrency == mRcvThreads.size());
      for (uint64_t i = 0; i < mConcurrency; ++i) {
        if (mRcvThreads[i].joinable()) {
          mRcvThreads[i].join();
        }
      }
    }
  }

  virtual void preSpawnCallData() {}

 protected:
  int mConcurrency;
  BlockingQueue<std::shared_ptr<Command>> &mCommandQueue;
  std::vector<std::unique_ptr<ServerCompletionQueue>> mCompletionQueues;
  AsyncService mService;

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

  std::string mReceiverName;
  std::string mIpPort;
  std::optional<TlsConf> mTlsConfOpt;
  std::unique_ptr<::grpc::Server> mServer;
  std::atomic<bool> mIsShutdown = false;
  std::vector<std::thread> mRcvThreads;
};

}  /// namespace gringofts::app
#endif  // SRC_APP_UTIL_MULTIREQUESTRECEIVER_H_

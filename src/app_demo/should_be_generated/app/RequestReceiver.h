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

#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_REQUESTRECEIVER_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_REQUESTRECEIVER_H_

#include <INIReader.h>
#include <grpcpp/grpcpp.h>

#include "../../../infra/es/Command.h"
#include "../../../infra/util/TlsUtil.h"
#include "../../generated/grpc/demo.grpc.pb.h"
#include "../domain/common_types.h"

using ::grpc::ServerCompletionQueue;

namespace gringofts {
namespace demo {

class RequestReceiver final {
 public:
  explicit RequestReceiver(const INIReader &,
                           BlockingQueue<std::shared_ptr<Command>> &);
  ~RequestReceiver() = default;

  // disallow copy ctor and copy assignment
  RequestReceiver(const RequestReceiver &) = delete;
  RequestReceiver &operator=(const RequestReceiver &) = delete;

  // disallow move ctor and move assignment
  RequestReceiver(RequestReceiver &&) = delete;
  RequestReceiver &operator=(RequestReceiver &&) = delete;

  void run();

  void shutdown();

 private:
  const uint64_t kConcurrency = 8;

  void handleRpcs(uint64_t i);

  BlockingQueue<std::shared_ptr<Command>> &mCommandQueue;

  protos::DemoService::AsyncService mService;
  std::unique_ptr<Server> mServer;
  std::atomic<bool> mIsShutdown = false;

  std::vector<std::unique_ptr<ServerCompletionQueue>> mCompletionQueues;
  std::vector<std::thread> mRcvThreads;

  std::string mIpPort;
  std::optional<TlsConf> mTlsConfOpt;
};

}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_REQUESTRECEIVER_H_

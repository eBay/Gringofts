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

#ifndef SRC_APP_UTIL_SPLIT_SPLITREQUESTRECEIVER_H_
#define SRC_APP_UTIL_SPLIT_SPLITREQUESTRECEIVER_H_

#include <INIReader.h>
#include <grpcpp/grpcpp.h>

#include "../../infra/es/Command.h"
#include "../../infra/util/TlsUtil.h"
#include "../generated/grpc/split.grpc.pb.h"

using ::grpc::ServerCompletionQueue;

namespace gringofts::app::split {

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

  void handleRpcs();

  BlockingQueue<std::shared_ptr<Command>> &mCommandQueue;

  gringofts::app::split::SplitService::AsyncService  mService;
  std::unique_ptr<::grpc::Server> mServer;
  std::atomic<bool> mIsShutdown = false;

  std::unique_ptr<ServerCompletionQueue> mCompletionQueue;

  std::string mIpPort;
  std::optional<TlsConf> mTlsConfOpt;
};

}  /// namespace gringofts::app

#endif  // SRC_APP_UTIL_SPLIT_SPLITREQUESTRECEIVER_H_

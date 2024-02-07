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

#pragma once

#include <string>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <vector>
#include <list>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <spdlog/spdlog.h>
#include "../common_types.h"
#include "../util/TimeUtil.h"
#include "../grpc/RequestHandle.h"

namespace gringofts {
namespace forward {

struct ForwardMetaBase {
  uint64_t mLeaderId;
  grpc::ServerContext* mServerContext;

  explicit ForwardMetaBase(uint64_t leaderId, grpc::ServerContext* serverContext = nullptr) :
      mLeaderId(leaderId), mServerContext(serverContext) {}
};

struct AsyncClientCallBase {
  virtual ~AsyncClientCallBase() = default;
  grpc::ClientContext mContext;
  grpc::Status mStatus;
  std::shared_ptr<ForwardMetaBase> mMeta;

  uint64_t mForwardRquestTime = 0;
  uint64_t mGotResponseTime = 0;
  uint64_t mReplyResponseTime = 0;

  virtual std::string toString() const = 0;
  virtual void handleResponse() = 0;
};
}  /// namespace forward
}  /// namespace gringofts

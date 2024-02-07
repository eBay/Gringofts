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
#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_REQUESTFORWARD_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_REQUESTFORWARD_H_
#include "../../../infra/forward/ForwardCore.h"
#include "../../generated/grpc/demo.grpc.pb.h"
#include "../../../app_util/AppInfo.h"
#include <spdlog/spdlog.h>

namespace gringofts::demo {

struct ExecuteForwardCall :
    public gringofts::forward::AsyncClientCall<protos::IncreaseResponse> {
  gringofts::RequestHandle *mClientHandle;

  explicit ExecuteForwardCall(gringofts::RequestHandle *clientHandle) :
      mClientHandle(clientHandle) {}

  std::string toString() const override {
    return "ExecuteForwardCall";
  }

  void handleResponse() override {
    if (!mStatus.ok()) {
      // forward failed, follower set code to 301
      mResponse.set_code(301);
      mResponse.set_message("Not a leader any longer");
    }
    if (mResponse.reserved().empty()) {
      mResponse.set_reserved(std::to_string(mMeta->mLeaderId));
    }
    mClientHandle->forwardResponseReply(&mResponse);
  }
};

typedef gringofts::forward::ForwardCore<protos::DemoService::Stub> ExecuteForwardCore;
}  // namespace gringofts::demo
#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_REQUESTFORWARD_H_

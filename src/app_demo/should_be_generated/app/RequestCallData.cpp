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

#include "RequestCallData.h"

#include "../../../app_util/AppInfo.h"
#include "../domain/IncreaseCommand.h"

namespace gringofts {
namespace demo {

RequestCallData::RequestCallData(
    protos::DemoService::AsyncService *service,
    ServerCompletionQueue *cq,
    BlockingQueue<std::shared_ptr<Command>> &commandQueue)
    : mService(service), mCompletionQueue(cq), mStatus(CREATE),
      mCommandQueue(commandQueue),
      mResponder(&mContext) {
  // Invoke the serving logic right away.
  proceed();
}

void RequestCallData::proceed() {
  if (mStatus == CREATE) {
    // Make this instance progress to the PROCESS state.
    mStatus = PROCESS;

    // As part of the initial CREATE state, we *request* that the system
    // start processing SayHello requests. In this request, "this" acts as
    // the tag uniquely identifying the request (so that different CallData
    // instances can serve different requests concurrently), in this case
    // the memory address of this CallData instance.
    mService->RequestExecute(&mContext, &mRequest, &mResponder,
                             mCompletionQueue, mCompletionQueue, this);
  } else if (mStatus == PROCESS) {
    // Spawn a new CallData instance to serve new clients while we process
    // the one for this CallData. The instance will deallocate itself as
    // part of its FINISH state.
    new RequestCallData(mService, mCompletionQueue, mCommandQueue);

    auto createdTimeInNanos = TimeUtil::currentTimeInNanos();
    auto command = std::make_shared<IncreaseCommand>(createdTimeInNanos, mRequest);
    command->setRequestHandle(this);
    command->setCreatorId(app::AppInfo::subsystemId());
    command->setGroupId(app::AppInfo::groupId());
    command->setGroupVersion(app::AppInfo::groupVersion());

    const std::string verifyResult = command->verifyCommand();
    if (verifyResult != Command::kVerifiedSuccess) {
      SPDLOG_WARN("Request can not pass validation due to Error: {} Request: {}",
                  verifyResult, mRequest.DebugString());
      fillResultAndReply(400, verifyResult, std::nullopt);
      return;
    }

    try {
      mCommandQueue.enqueue(std::move(command));
    }
    catch (const QueueStoppedException &e) {
      SPDLOG_WARN(e.what());
      fillResultAndReply(503, std::string(e.what()), std::nullopt);
    }
  } else {
    GPR_ASSERT(mStatus == FINISH);
    // Once in the FINISH state, deallocate ourselves (CallData).
    delete this;
  }
}

void RequestCallData::fillResultAndReply(uint32_t code,
                                         const std::string &message,
                                         std::optional<uint64_t> leaderId) {
  mResponse.set_code(code);
  mResponse.set_message(message);

  if (code == 301 && leaderId) {
    mResponse.set_reserved(std::to_string(*leaderId));
  }

  if (!(code == 200 || code == 201 || code == 301 || code == 400 || code == 503)) {
    mResponse.set_code(503);
  }

  mStatus = FINISH;
  mResponder.Finish(mResponse, Status::OK, this);
}

}  /// namespace demo
}  /// namespace gringofts

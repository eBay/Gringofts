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

#include "SplitCommand.h"

#include "RequestCallData.h"

namespace gringofts::app::split {

SplitCommand::SplitCommand(TimestampInNanos createdTimeInNanos, const gringofts::app::split::SplitRequest &request)
    : Command(SPILT_COMMAND, createdTimeInNanos), mRequest(request) {
}

SplitCommand::SplitCommand(TimestampInNanos createdTimeInNanos, const std::string &requestStr)
    : Command(SPILT_COMMAND, createdTimeInNanos) {
  decodeFromString(requestStr);
}

void SplitCommand::onPersisted(const std::string &message) {
  auto *callData = getRequestHandle();
  if (callData == nullptr) {
    SPDLOG_WARN("This command does not have request attached.");
    return;
  }
  callData->fillResultAndReply(200, message, std::nullopt);
}

void SplitCommand::onPersistFailed(const std::string &errorMessage, std::optional<uint64_t> reserved) {
  auto *callData = getRequestHandle();
  if (callData == nullptr) {
    SPDLOG_WARN("This command does not have request attached.");
    return;
  }
  callData->fillResultAndReply(503, errorMessage, reserved);
}

}  ///  namespace gringofts::app

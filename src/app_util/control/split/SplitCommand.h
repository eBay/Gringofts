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

#ifndef SRC_APP_UTIL_CONTROL_SPLIT_SPLITCOMMAND_H_
#define SRC_APP_UTIL_CONTROL_SPLIT_SPLITCOMMAND_H_


#include "../CtrlCommandEvent.h"
#include "../../generated/grpc/scale.pb.h"

namespace gringofts::app::ctrl::split {

const Type SPILT_COMMAND = 101;

/**
 * This is the command for execution plan.  For now, it support protobuf
 * for journal and account request. But for execution plans which has predicates,
 * the protobuf does not work yet.
 */
class SplitCommand : public CtrlCommand {
 public:
  using Request = protos::Scale::SplitRequest;
  SplitCommand(TimestampInNanos createdTimeInNanos, const Request &request)
      : CtrlCommand(SPILT_COMMAND, createdTimeInNanos), mRequest(request) {
  }

  SplitCommand(TimestampInNanos createdTimeInNanos, const std::string_view &requestStr)
      : CtrlCommand(SPILT_COMMAND, createdTimeInNanos) {
    decodeFromString(std::string(requestStr));
  }

  std::string_view dedupId() const override { return mRequest.planid(); }

  std::string encodeToString() const override { return mRequest.SerializeAsString(); }

  std::optional<std::string> specialTag() const override { return mRequest.planid(); }

  void decodeFromString(std::string_view encodedString) override {
    mRequest.ParseFromString(std::string(encodedString));
  }

  std::string verifyCommand() const override;

  ProcessHint process(const CtrlState &state, std::vector<std::shared_ptr<Event>> *events) const override;

 private:
  Request mRequest;
};

}  // namespace gringofts::app::ctrl::split

#endif  // SRC_APP_UTIL_CONTROL_SPLIT_SPLITCOMMAND_H_

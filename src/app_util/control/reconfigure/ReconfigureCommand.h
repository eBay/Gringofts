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

#ifndef SRC_APP_UTIL_CONTROL_RECONFIGURE_RECONFIGURECOMMAND_H_
#define SRC_APP_UTIL_CONTROL_RECONFIGURE_RECONFIGURECOMMAND_H_

#include "../CtrlCommandEvent.h"

namespace gringofts::app::ctrl::reconfigure {

const Type RECONFIGURE_COMMAND = 103;

class ReconfigureCommand : public CtrlCommand {
 public:
  using Request = protos::Reconfigure::Request;
  ReconfigureCommand(TimestampInNanos createdTimeInNanos, const Request &request)
      : CtrlCommand(RECONFIGURE_COMMAND, createdTimeInNanos), mRequest(request) {}
  ReconfigureCommand(TimestampInNanos createdTimeInNanos, const std::string_view &requestStr)
      : CtrlCommand(RECONFIGURE_COMMAND, createdTimeInNanos) {
    decodeFromString(std::string(requestStr));
  }

  std::string encodeToString() const override { return mRequest.SerializeAsString(); }
  void decodeFromString(std::string_view encodedString) override {
    mRequest.ParseFromString(std::string(encodedString));
  }

  std::optional<std::string> specialTag() const override { return std::to_string(mRequest.version()); }

  std::string verifyCommand() const override;
  ProcessHint process(const CtrlState &state, std::vector<std::shared_ptr<Event>> *events) const override;

 private:
  Request mRequest;
};
}  // namespace gringofts::app::ctrl::reconfigure

#endif  // SRC_APP_UTIL_CONTROL_RECONFIGURE_RECONFIGURECOMMAND_H_

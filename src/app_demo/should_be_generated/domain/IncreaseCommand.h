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

#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_INCREASECOMMAND_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_INCREASECOMMAND_H_

#include "../../../infra/es/Command.h"
#include "../../../infra/es/ProcessCommandStateMachine.h"
#include "../../generated/grpc/demo.pb.h"

namespace gringofts {
namespace demo {

/**
 * This is the command for execution plan.  For now, it support protobuf
 * for journal and account request. But for execution plans which has predicates,
 * the protobuf does not work yet. We will enable it when we work on Legos book.
 */
class IncreaseCommand : public Command {
 public:
  IncreaseCommand(TimestampInNanos, const protos::IncreaseRequest &);

  IncreaseCommand(TimestampInNanos, const std::string &);

  std::string encodeToString() const override {
    return mRequest.SerializeAsString();
  }

  void decodeFromString(std::string_view encodedString) override {
    mRequest.ParseFromString(std::string(encodedString));
  }

  int getValue() const {
    return mRequest.value();
  }

  protos::IncreaseRequest getRequest() const {
    return mRequest;
  }

  std::string verifyCommand() const override {
    return kVerifiedSuccess;
  }

 private:
  void onPersisted(const std::string &message) override;
  void onPersistFailed(const std::string &errorMessage, std::optional<uint64_t> reserved) override;

  protos::IncreaseRequest mRequest;
};

}  ///  namespace demo
}  ///  namespace gringofts

#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_INCREASECOMMAND_H_

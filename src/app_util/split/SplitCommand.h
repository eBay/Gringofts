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

#ifndef SRC_APP_UTIL_SPLITCOMMAND_H_
#define SRC_APP_UTIL_SPLITCOMMAND_H_

#include "../../infra/es/Command.h"
#include "../../infra/es/ProcessCommandStateMachine.h"
#include "../generated/grpc/split.grpc.pb.h"

namespace gringofts::app::split {

const Type SPILT_COMMAND = 100;

/**
 * This is the command for execution plan.  For now, it support protobuf
 * for journal and account request. But for execution plans which has predicates,
 * the protobuf does not work yet. We will enable it when we work on Legos book.
 */
class SplitCommand : public Command {
 public:
  SplitCommand(TimestampInNanos, const gringofts::app::split::SplitRequest&);

  SplitCommand(TimestampInNanos, const std::string &);

  std::string encodeToString() const override {
    return mRequest.SerializeAsString();
  }

  void decodeFromString(std::string_view encodedString) override {
    mRequest.ParseFromString(std::string(encodedString));
  }


  gringofts::app::split::SplitRequest getRequest() const {
    return mRequest;
  }

  std::string verifyCommand() const override {
    return kVerifiedSuccess;
  }

 private:
  void onPersisted(const std::string &message) override;
  void onPersistFailed(const std::string &errorMessage, std::optional<uint64_t> reserved) override;

  gringofts::app::split::SplitRequest mRequest;
};

}  ///  namespace gringofts::app

#endif  // SRC_APP_UTIL_SPLITCOMMAND_H_

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

#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_PROCESSEDEVENT_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_PROCESSEDEVENT_H_

#include "../../../infra/es/Event.h"

#include "../../generated/grpc/demo.pb.h"
#include "common_types.h"

namespace gringofts {
namespace demo {

/**
 * This event is created to record journal line creation.
 */
class ProcessedEvent : public Event {
 public:
  ProcessedEvent(TimestampInNanos createdTimeInNanos, const protos::IncreaseRequest &request);

  ProcessedEvent(TimestampInNanos createdTimeInNanos, std::string_view journalString);

  std::string encodeToString() const override;

  void decodeFromString(std::string_view payload) override;

  int getValue() const {
    return mRequest.value();
  }

 private:
  protos::IncreaseRequest mRequest;
};

}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_PROCESSEDEVENT_H_

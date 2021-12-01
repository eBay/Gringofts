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

#ifndef SRC_APP_UTIL_CONTROL_SPLIT_SPLITEVENT_H_
#define SRC_APP_UTIL_CONTROL_SPLIT_SPLITEVENT_H_

#include "../../generated/grpc/scale.pb.h"
#include "../CtrlCommandEvent.h"
#include "../CtrlState.h"

namespace gringofts::app::ctrl::split {

const Type SPILT_EVENT = 101;

/**
 * This event is created to record journal line creation.
 */
class SplitEvent : public CtrlEvent {
 public:
  using Request = protos::Scale::SplitRequest;
  SplitEvent(TimestampInNanos createdTimeInNanos, const Request &state)
      : CtrlEvent(SPILT_EVENT, createdTimeInNanos), mRequest(state) {}

  SplitEvent(TimestampInNanos createdTimeInNanos, std::string_view requestString)
      : CtrlEvent(SPILT_EVENT, createdTimeInNanos) {
    decodeFromString(requestString);
  }

  std::string encodeToString() const override { return mRequest.SerializeAsString(); }

  void decodeFromString(std::string_view payload) override { mRequest.ParseFromString(std::string(payload)); }

  void apply(CtrlState *state) const override;

 private:
  Request mRequest;
};

}  // namespace gringofts::app::ctrl::split

#endif  // SRC_APP_UTIL_CONTROL_SPLIT_SPLITEVENT_H_

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

#ifndef SRC_APP_UTIL_CONTROL_RECONFIGURE_RECONFIGUREEVENT_H_
#define SRC_APP_UTIL_CONTROL_RECONFIGURE_RECONFIGUREEVENT_H_

#include <map>
#include "../CtrlCommandEvent.h"
#include "../../../infra/util/ClusterInfo.h"

namespace gringofts::app::ctrl::reconfigure {

const Type RECONFIGURE_EVENT = 103;

class AppStateMachine;

class ReconfigureEvent : public CtrlEvent {
 public:
  using Request = gringofts::app::protos::Reconfigure::Request;
  ReconfigureEvent(TimestampInNanos createdTimeInNanos, const Request &request)
      : CtrlEvent(RECONFIGURE_EVENT, createdTimeInNanos), mRequest(request) {
        syncFromProto();
      }
  ReconfigureEvent(TimestampInNanos createdTimeInNanos, std::string_view requestString)
      : CtrlEvent(RECONFIGURE_EVENT, createdTimeInNanos) {
        decodeFromString(requestString);
      }

  std::string encodeToString() const override {
    return mRequest.SerializeAsString();
  }

  void decodeFromString(std::string_view payload) override {
    mRequest.ParseFromString(std::string(payload));
    syncFromProto();
  }

  void apply(CtrlState *state) const override;

  void onApplied() const override;

 private:
  void syncFromProto();

  // fields to be serialized
  mutable Request mRequest;

  // event related fields in memory;
  uint64_t mClusterVersion;
  ClusterInfo mClusterConfiguration;
};

}  // namespace gringofts::app::ctrl::reconfigure

#endif  // SRC_APP_UTIL_CONTROL_RECONFIGURE_RECONFIGUREEVENT_H_

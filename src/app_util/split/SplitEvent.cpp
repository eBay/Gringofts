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

#include "SplitEvent.h"

#include <spdlog/spdlog.h>

namespace gringofts::app::split {

SplitEvent::SplitEvent(TimestampInNanos createdTimeInNanos, const gringofts::app::split::SplitRequest &request)
    : Event(SPILT_EVENT, createdTimeInNanos), mRequest(std::move(request)) {}

SplitEvent::SplitEvent(TimestampInNanos createdTimeInNanos, std::string_view requestString)
    : Event(SPILT_EVENT, createdTimeInNanos) {
  decodeFromString(requestString);
}

std::string SplitEvent::encodeToString() const {
  return mRequest.SerializeAsString();
}

void SplitEvent::decodeFromString(std::string_view payload) {
  mRequest.ParseFromString(std::string(payload));
}

}  /// namespace gringofts::app

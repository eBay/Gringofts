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

#include "ProcessedEvent.h"

#include <spdlog/spdlog.h>

namespace gringofts {
namespace demo {

ProcessedEvent::ProcessedEvent(TimestampInNanos createdTimeInNanos, const protos::IncreaseRequest &request)
    : Event(PROCESSED_EVENT, createdTimeInNanos), mRequest(std::move(request)) {}

ProcessedEvent::ProcessedEvent(TimestampInNanos createdTimeInNanos, std::string_view requestString)
    : Event(PROCESSED_EVENT, createdTimeInNanos) {
  decodeFromString(requestString);
}

std::string ProcessedEvent::encodeToString() const {
  return mRequest.SerializeAsString();
}

void ProcessedEvent::decodeFromString(std::string_view payload) {
  mRequest.ParseFromString(std::string(payload));
}

}  /// namespace demo
}  /// namespace gringofts

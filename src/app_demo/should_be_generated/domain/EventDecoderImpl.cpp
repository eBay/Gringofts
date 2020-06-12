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

#include "EventDecoderImpl.h"
#include "ProcessedEvent.h"

namespace gringofts {
namespace demo {

std::unique_ptr<Event> EventDecoderImpl::decodeEventFromString(const EventMetaData &metaData,
                                                               std::string_view payload) const {
  std::unique_ptr<Event> event;
  switch (metaData.getType()) {
    case PROCESSED_EVENT: {
      event = std::make_unique<ProcessedEvent>(metaData.getCreatedTimeInNanos(), std::string(payload));
      break;
    }
    default:return nullptr;
  }
  event->setPartialMetaData(metaData);
  return std::move(event);
}

}  /// namespace demo
}  /// namespace gringofts

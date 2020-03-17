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

#ifndef SRC_INFRA_ES_EVENTDECODER_H_
#define SRC_INFRA_ES_EVENTDECODER_H_

#include "../common_types.h"
#include "Event.h"

namespace gringofts {

/**
 * An interface which provides methods to decode ::gringofts::Event from different formats, e.g., string, binary
 */
class EventDecoder {
 public:
  virtual ~EventDecoder() = default;

  /**
   * Decode ::gringofts::Event from string
   * @param metaData metadata of this event
   * @param encodedString string with which to decode to get ::gringofts::Event
   * @return a unique pointer to the decoded in-memory ::gringofts::Event instance or *nullptr* if failed
   */
  virtual std::unique_ptr<Event> decodeEventFromString(const EventMetaData &metaData,
                                                       std::string_view encodedString) const = 0;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_EVENTDECODER_H_

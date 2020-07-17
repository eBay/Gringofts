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

#ifndef SRC_INFRA_ES_STORE_COMMANDEVENTDECODEWRAPPER_H_
#define SRC_INFRA_ES_STORE_COMMANDEVENTDECODEWRAPPER_H_

#include "../CommandDecoder.h"
#include "../EventDecoder.h"
#include "generated/store.pb.h"

namespace gringofts {

/**
 * This class wraps the decoding process for CommandEntry->Command, and EventEntry->Event.
 * It will be used in both raft-backed store and snapshot.
 */
class CommandEventDecodeWrapper {
 public:
  static std::unique_ptr<Command> decodeCommand(const es::CommandEntry &commandEntry,
                                                const CommandDecoder &commandDecoder) {
    return commandDecoder.decodeCommandFromString(CommandMetaData{commandEntry},
                                                  std::string_view{commandEntry.entry()});
  }

  static std::unique_ptr<Event> decodeEvent(const es::EventEntry &eventEntry,
                                            const EventDecoder &eventDecoder) {
    return eventDecoder.decodeEventFromString(EventMetaData{eventEntry},
                                              std::string_view{eventEntry.entry()});
  }
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_STORE_COMMANDEVENTDECODEWRAPPER_H_

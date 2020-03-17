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

#ifndef SRC_INFRA_ES_STORE_COMMANDEVENTENCODEWRAPPER_H_
#define SRC_INFRA_ES_STORE_COMMANDEVENTENCODEWRAPPER_H_

#include "generated/store.pb.h"
#include "../Command.h"

namespace gringofts {

/**
 * This class wraps the encoding process for Command->CommandEntry, and Event->EventEntry.
 * It will be used in both raft-backed store and snapshot.
 */
class CommandEventEncodeWrapper {
 public:
  static void encodeCommand(const Command &command, es::CommandEntry *commandEntry) {
    command.getMetaData().populateCommandEntry(commandEntry);
    commandEntry->set_entry(command.encodeToString());
  }

  static void encodeEvent(const Event &event, es::EventEntry *eventEntry) {
    event.getMetaData().populateEventEntry(eventEntry);
    eventEntry->set_entry(event.encodeToString());
  }
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_STORE_COMMANDEVENTENCODEWRAPPER_H_

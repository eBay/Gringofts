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

#ifndef SRC_APP_UTIL_COMMANDEVENTDECODERIMPL_H_
#define SRC_APP_UTIL_COMMANDEVENTDECODERIMPL_H_

#include "../infra/common_types.h"
#include "../infra/es/CommandEventDecoder.h"
#include "split/SplitCommand.h"
#include "split/SplitEvent.h"

namespace gringofts {
namespace app {

/**
 * A wrapper class which provides a one-stop place for decoding #gringofts::Command and #gringofts::Event.
 * @tparam EventDecoderType type of event decoder
 * @tparam CommandDecoderType type of command decoder
 */
template<typename EventDecoderType, typename CommandDecoderType>
class CommandEventDecoderImpl : public CommandEventDecoder {
 public:
  CommandEventDecoderImpl() = default;
  ~CommandEventDecoderImpl() = default;

  std::unique_ptr<Event> decodeEventFromString(const EventMetaData &metaData,
                                               std::string_view payload) const override {

    switch (metaData.getType()) {
      case split::SPILT_EVENT: {
        auto command = std::make_unique<split::SplitEvent>(metaData.getCreatedTimeInNanos(), std::string(payload));
        command->setPartialMetaData(metaData);
        return command;
      }
    }
    return mEventDecoderImpl.decodeEventFromString(metaData, payload);
  }

  std::unique_ptr<Command> decodeCommandFromString(const CommandMetaData &metaData,
                                                   std::string_view payload) const override {
    switch (metaData.getType()) {
      case split::SPILT_COMMAND: {
        auto event = std::make_unique<split::SplitCommand>(metaData.getCreatedTimeInNanos(), std::string(payload));
        event->setPartialMetaData(metaData);
        return event;
      }
    }
    return mCommandDecoderImpl.decodeCommandFromString(metaData, payload);
  }

 private:
  EventDecoderType mEventDecoderImpl;
  CommandDecoderType mCommandDecoderImpl;
};

}  /// namespace app
}  /// namespace gringofts

#endif  // SRC_APP_UTIL_COMMANDEVENTDECODERIMPL_H_

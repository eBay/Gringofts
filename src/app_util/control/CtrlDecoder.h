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

#ifndef SRC_APP_UTIL_CONTROL_CTRLDECODER_H_
#define SRC_APP_UTIL_CONTROL_CTRLDECODER_H_

#include "CtrlCommandEvent.h"
#include "split/SplitCommand.h"
#include "split/SplitEvent.h"

namespace gringofts::app::ctrl {
class CtrlDecoderUtil {
 public:
  static std::unique_ptr<CtrlEvent> decodeEvent(const gringofts::EventMetaData &metaData,
                                                std::string_view payload) {
    switch (metaData.getType()) {
      case split::SPILT_EVENT: {
        auto ptr = std::make_unique<split::SplitEvent>(metaData.getCreatedTimeInNanos(), payload);
        ptr->setPartialMetaData(metaData);
        return ptr;
      }
      default:return std::unique_ptr<CtrlEvent>{};
    }
  }

  static std::unique_ptr<CtrlCommand> decodeCommand(const gringofts::CommandMetaData &metaData,
                                                    std::string_view payload) {
    switch (metaData.getType()) {
      case split::SPILT_COMMAND: {
        auto ptr = std::make_unique<split::SplitCommand>(metaData.getCreatedTimeInNanos(), payload);
        ptr->setPartialMetaData(metaData);
        return ptr;
      }
      default:return std::unique_ptr<CtrlCommand>{};
    }
  }
};
}  // namespace gringofts::app::ctrl

#endif  // SRC_APP_UTIL_CONTROL_CTRLDECODER_H_

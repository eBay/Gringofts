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

#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_COMMANDDECODERIMPL_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_COMMANDDECODERIMPL_H_

#include "../../../infra/es/CommandDecoder.h"

namespace gringofts {
namespace demo {

class CommandDecoderImpl : public CommandDecoder {
 public:
  CommandDecoderImpl() = default;
  ~CommandDecoderImpl() = default;

  std::unique_ptr<Command> decodeCommandFromString(const CommandMetaData &,
                                                   std::string_view) const override;
};

}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_COMMANDDECODERIMPL_H_

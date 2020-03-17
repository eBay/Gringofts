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

#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_COMMON_TYPES_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_COMMON_TYPES_H_

#include <spdlog/spdlog.h>

#include "../../../infra/common_types.h"
#include "../../../infra/es/Command.h"
#include "../../../infra/es/Event.h"

#include "../../generated/grpc/demo.pb.h"

namespace gringofts {
namespace demo {

// es interface related
using ::gringofts::Command;

// grpc-related
using ::grpc::Server;
using ::grpc::ServerContext;
using ::grpc::ServerBuilder;
using ::grpc::Status;

using ::grpc::Channel;
using ::grpc::ClientContext;

// event types
const Type PROCESSED_EVENT = 6;
// command types
const Type INCREASE_COMMAND = 0;

namespace protobuf = ::google::protobuf;

// @formatter:off
using CommandEventsEntry = std::tuple<std::shared_ptr<Command>, std::vector<std::shared_ptr<Event>>>;
// @formatter:on
using CommandEventQueue = BlockingQueue<CommandEventsEntry>;

}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_COMMON_TYPES_H_

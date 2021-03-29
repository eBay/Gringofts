/************************************************************************
Copyright 2021-2022 eBay Inc.
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


#include <rocksdb/db.h>
#include <spdlog/spdlog.h>

#include "ConnectCommand.h"

namespace goblin::kvengine::model {

ConnectCommandContext::ConnectCommandContext(
    const proto::Connect::Request& req,
    AsyncConnectCBFunc cb) : mRequest(req), mCB(cb) {
}

void ConnectCommandContext::initSuccessResponse(
    const store::VersionType &curMaxVersion,
    const model::EventList &events) {
  assert(events.empty());
  mResponse.mutable_header()->set_code(proto::ResponseCode::OK);
  mResponse.mutable_header()->set_latestversion(curMaxVersion);
}

void ConnectCommandContext::fillResponseAndReply(
    proto::ResponseCode code,
    const std::string &message,
    std::optional<uint64_t> leaderId) {
  SPDLOG_INFO("debug: connect fill response");
  if (code == proto::ResponseCode::NOT_LEADER && leaderId) {
    mResponse.mutable_header()->set_leaderhint(std::to_string(*leaderId));
  }
  mResponse.mutable_header()->set_code(code);
  mResponse.mutable_header()->set_message(message);
  mCB(mResponse);
}

void ConnectCommandContext::reportSubMetrics() {
  /// metrics counter
  auto connectCounter = gringofts::getCounter("connect_command_counter", {});
  connectCounter.increase();
}

const proto::Connect::Request& ConnectCommandContext::getRequest() {
  return mRequest;
}

std::set<store::KeyType> ConnectCommandContext::getTargetKeys() {
  return {};
}

proto::RequestHeader ConnectCommandContext::getRequestHeader() {
  return mRequest.header();
}

ConnectCommand::ConnectCommand(std::shared_ptr<ConnectCommandContext> context):
  mContext(context) {
}

std::shared_ptr<CommandContext> ConnectCommand::getContext() {
  return mContext;
}

utils::Status ConnectCommand::prepare(const std::shared_ptr<store::KVStore> &) {
  /// SPDLOG_INFO("debug: prepare connect command");
  /// no events needed
  return utils::Status::ok();
}

utils::Status ConnectCommand::execute(const std::shared_ptr<store::KVStore> &kvStore, EventList *events) {
  /// SPDLOG_INFO("debug: execute connect command");
  /// no events needed
  return utils::Status::ok();
}

utils::Status ConnectCommand::finish(const std::shared_ptr<store::KVStore> &kvStore, const EventList &events) {
  /// SPDLOG_INFO("debug: finish connect command");
  /// no events needed
  return utils::Status::ok();
}

}  // namespace goblin::kvengine::model

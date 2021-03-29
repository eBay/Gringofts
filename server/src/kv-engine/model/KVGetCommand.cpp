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


#include "KVGetCommand.h"

#include <rocksdb/db.h>
#include <spdlog/spdlog.h>

#include "../utils/StrUtil.h"

namespace goblin::kvengine::model {

KVGetCommandContext::KVGetCommandContext(const proto::Get::Request& req, AsyncGetCBFunc cb) : mRequest(req), mCB(cb) {
}

void KVGetCommandContext::initSuccessResponse(const store::VersionType &curMaxVersion, const model::EventList &events) {
  mResponse.mutable_result()->set_code(proto::ResponseCode::OK);
  mResponse.mutable_header()->set_code(proto::ResponseCode::OK);
  if (events.size() > 0) {
    assert(events.size() == 1);
    auto &e = events[0];
    assert(e->getType() == model::EventType::READ);
    auto *read = dynamic_cast<const model::ReadEvent*>(e.get());
    assert(read != nullptr);
    mResponse.mutable_result()->set_value(read->getValue());
    mResponse.mutable_result()->set_version(read->getValueVersion());
    if (read->isNotFound()) {
      mResponse.mutable_result()->set_code(proto::ResponseCode::KEY_NOT_EXIST);
    }
  }
  mResponse.mutable_header()->set_latestversion(curMaxVersion);
}

void KVGetCommandContext::fillResponseAndReply(
    proto::ResponseCode code,
    const std::string &message,
    std::optional<uint64_t> leaderId) {
  SPDLOG_INFO("debug: get fill response {}", code);
  if (code == proto::ResponseCode::NOT_LEADER && leaderId) {
    mResponse.mutable_header()->set_leaderhint(std::to_string(*leaderId));
  } else if (code == proto::ResponseCode::WRONG_ROUTE) {
    std::vector<std::string> addrs = utils::StrUtil::tokenize(message, ',');
    assert(!addrs.empty());
    for (auto addr : addrs) {
      auto server = mResponse.mutable_header()->mutable_routehint()->add_servers();
      server->set_hostname(addr);
    }
  }
  mResponse.mutable_header()->set_code(code);
  if (code != proto::ResponseCode::OK) {
    mResponse.mutable_result()->set_code(code);
  }
  mResponse.mutable_header()->set_message(message);
  mCB(mResponse);
}

void KVGetCommandContext::reportSubMetrics() {
  /// metrics counter
  auto getCounter = gringofts::getCounter("get_command_counter", {});
  getCounter.increase();
  /// SPDLOG_INFO("debug: read op");
}

const proto::Get::Request& KVGetCommandContext::getRequest() {
  return mRequest;
}

std::set<store::KeyType> KVGetCommandContext::getTargetKeys() {
  return {mRequest.entry().key()};
}

proto::RequestHeader KVGetCommandContext::getRequestHeader() {
  return mRequest.header();
}

KVGetCommand::KVGetCommand(std::shared_ptr<KVGetCommandContext> context):
  mContext(context) {
}

std::shared_ptr<CommandContext> KVGetCommand::getContext() {
  return mContext;
}

utils::Status KVGetCommand::prepare(const std::shared_ptr<store::KVStore> &kvStore) {
  /// acquire shared lock
  return kvStore->lock(mContext->getTargetKeys(), false);
}

utils::Status KVGetCommand::execute(const std::shared_ptr<store::KVStore> &kvStore, EventList *events) {
  auto &req = mContext->getRequest();
  const store::KeyType &key = req.entry().key();
  store::ValueType value;
  store::TTLType ttl = store::INFINITE_TTL;
  store::VersionType version = store::VersionStore::kInvalidVersion;
  /// the value we read may not be committed at this moment
  /// but the ReadEvent generated will wait until it it commited and reply to clients
  auto s = kvStore->readKV(key, &value, &ttl, &version);
  SPDLOG_INFO("debug: executing get cmd, key {}, value {}, newVersion {}", key, value, version);
  if (s.isNotFound()) {
    events->push_back(std::make_shared<ReadEvent>(key, value, ttl, version, true));
  } else if (s.isOK()) {
    events->push_back(std::make_shared<ReadEvent>(key, value, ttl, version, false));
  }
  return s;
}

utils::Status KVGetCommand::finish(const std::shared_ptr<store::KVStore> &kvStore, const EventList &events) {
  auto s = utils::Status::ok();
  for (auto &e : events) {
    s = e->apply(*kvStore);
    if (!s.isOK()) {
      break;
    }
  }
  assert(kvStore->unlock(mContext->getTargetKeys(), false).isOK());
  return s;
}

}  // namespace goblin::kvengine::model

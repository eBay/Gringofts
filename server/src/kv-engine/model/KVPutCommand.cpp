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


#include "KVPutCommand.h"

#include <rocksdb/db.h>
#include <spdlog/spdlog.h>

#include "../utils/StrUtil.h"

namespace goblin::kvengine::model {

KVPutCommandContext::KVPutCommandContext(const proto::Put::Request& req, AsyncPutCBFunc cb) : mRequest(req), mCB(cb) {
}

void KVPutCommandContext::initSuccessResponse(const store::VersionType &curMaxVersion, const model::EventList &events) {
  mResponse.mutable_result()->set_code(proto::ResponseCode::OK);
  mResponse.mutable_header()->set_code(proto::ResponseCode::OK);
  auto latestVersion = curMaxVersion;
  if (events.size() > 0) {
    assert(events.size() == 1);
    auto &e = events[0];
    assert(e->getType() == model::EventType::WRITE);
    auto *write = dynamic_cast<const model::WriteEvent*>(e.get());
    assert(write != nullptr);
    auto allocatedVersion = write->getAllocatedVersion();
    mResponse.mutable_result()->set_version(allocatedVersion);
    latestVersion = std::max(allocatedVersion, curMaxVersion);
  }
  mResponse.mutable_header()->set_latestversion(latestVersion);
  mResponse.mutable_header()->set_code(proto::ResponseCode::OK);
}

void KVPutCommandContext::fillResponseAndReply(
    proto::ResponseCode code,
    const std::string &message,
    std::optional<uint64_t> leaderId) {
  SPDLOG_INFO("debug: put fill response");
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

void KVPutCommandContext::reportSubMetrics() {
  /// metrics counter
  auto putCounter = gringofts::getCounter("put_command_counter", {});
  putCounter.increase();
  /// SPDLOG_INFO("debug: write op");
}

const proto::Put::Request& KVPutCommandContext::getRequest() {
  return mRequest;
}

std::set<store::KeyType> KVPutCommandContext::getTargetKeys() {
  return {mRequest.entry().key()};
}

proto::RequestHeader KVPutCommandContext::getRequestHeader() {
  return mRequest.header();
}

KVPutCommand::KVPutCommand(std::shared_ptr<KVPutCommandContext> context):
  mContext(context) {
}

std::shared_ptr<CommandContext> KVPutCommand::getContext() {
  return mContext;
}

utils::Status KVPutCommand::prepare(const std::shared_ptr<store::KVStore> &kvStore) {
  /// acquire exclusive lock
  return kvStore->lock(mContext->getTargetKeys(), true);
}

utils::Status KVPutCommand::execute(const std::shared_ptr<store::KVStore> &kvStore, EventList *events) {
  using store::INFINITE_TTL;
  using store::NEVER_EXPIRE;
  using utils::TimeUtil;

  auto &req = mContext->getRequest();
  const store::KeyType &key = req.entry().key();
  const store::ValueType &value = req.entry().value();

  bool enableTTL = req.entry().enablettl();
  store::TTLType ttl = enableTTL? req.entry().ttl() : INFINITE_TTL;

  SPDLOG_INFO("debug: executing put cmd, key {}, value {}, ttl {}, enableTTL {}", key, value, ttl, enableTTL);
  utils::Status s = utils::Status::ok();
  /// fix ttl value if not set while ttl enabled
  if (enableTTL && ttl == INFINITE_TTL) {
    proto::Meta meta;
    s = kvStore->readMeta(key, &meta);
    if (s.isOK()) {
      if (meta.ttl() != INFINITE_TTL && meta.deadline() > TimeUtil::secondsSinceEpoch()) {
        ttl = meta.ttl();
      } else {
        enableTTL = false;
      }
    } else if (s.isNotFound()) {
      /// Keep the value forever if previous ttl not found or value is expired
      enableTTL = false;
    } else {
      return s;
    }
  }

  const utils::TimeType &deadline = enableTTL? ttl + TimeUtil::secondsSinceEpoch() : NEVER_EXPIRE;

  events->push_back(std::make_shared<WriteEvent>(key, value, enableTTL, ttl, deadline));
  return utils::Status::ok();
}

utils::Status KVPutCommand::finish(const std::shared_ptr<store::KVStore> &kvStore, const EventList &events) {
  auto s = utils::Status::ok();
  for (auto &e : events) {
    s = e->apply(*kvStore);
    if (!s.isOK()) {
      break;
    }
  }
  assert(kvStore->unlock(mContext->getTargetKeys(), true).isOK());
  return s;
}

}  /// namespace goblin::kvengine::model

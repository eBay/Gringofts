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


#include "KVGenerateCommand.h"

#include <absl/strings/str_cat.h>
#include <rocksdb/db.h>
#include <spdlog/spdlog.h>

#include "../utils/StrUtil.h"

namespace goblin::kvengine::model {

KVGenerateCommandContext::KVGenerateCommandContext(
    const proto::GenerateKV::Request& req,
    AsyncGenerateCBFunc cb) : mRequest(req), mCB(cb) {
}

void KVGenerateCommandContext::initSuccessResponse(
    const store::VersionType &curMaxVersion,
    const model::EventList &events) {
  mResponse.mutable_result()->set_code(proto::ResponseCode::OK);
  mResponse.mutable_header()->set_code(proto::ResponseCode::OK);
  auto latestVersion = curMaxVersion;
  for (auto &e : events) {
    assert(e->getType() == model::EventType::WRITE);
    auto *write = dynamic_cast<const model::WriteEvent*>(e.get());
    assert(write != nullptr);
  }
  mResponse.mutable_result()->set_outputinfo(mOutputInfo);
  mResponse.mutable_header()->set_latestversion(latestVersion);
}

void KVGenerateCommandContext::fillResponseAndReply(
    proto::ResponseCode code,
    const std::string &message,
    std::optional<uint64_t> leaderId) {
  SPDLOG_INFO("debug: generate fill response");
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

void KVGenerateCommandContext::reportSubMetrics() {
  /// metrics counter
  auto generateCounter = gringofts::getCounter("generate_command_counter", {});
  generateCounter.increase();
  /// SPDLOG_INFO("debug: write op");
}

const proto::GenerateKV::Request& KVGenerateCommandContext::getRequest() {
  return mRequest;
}

std::map<store::KeyType, std::pair<store::ValueType, store::VersionType>> *KVGenerateCommandContext::getKVsToRead() {
  return &mKVsToRead;
}

std::map<store::KeyType, store::ValueType> *KVGenerateCommandContext::getKVsToWrite() {
  return &mKVsToWrite;
}

OutputInfoType *KVGenerateCommandContext::getOutputInfo() {
  return &mOutputInfo;
}

std::set<store::KeyType> KVGenerateCommandContext::getTargetKeys() {
  std::set<store::KeyType> keys;
  for (auto &k : mRequest.entry().srckeys()) {
    keys.insert(k);
  }
  for (auto &k : mRequest.entry().tgtkeys()) {
    keys.insert(k);
  }
  return keys;
}

proto::RequestHeader KVGenerateCommandContext::getRequestHeader() {
  return mRequest.header();
}

KVGenerateCommand::KVGenerateCommand(std::shared_ptr<KVGenerateCommandContext> context):
  mContext(context) {
}

std::shared_ptr<CommandContext> KVGenerateCommand::getContext() {
  return mContext;
}

utils::Status KVGenerateCommand::prepare(const std::shared_ptr<store::KVStore> &kvStore) {
  /// 1. fill src kv from store
  auto srcKeys = mContext->getRequest().entry().srckeys();
  auto kvsToRead = mContext->getKVsToRead();
  for (auto &key : srcKeys) {
    store::ValueType value;
    store::TTLType ttl;
    store::VersionType version;
    auto s = kvStore->readKV(key, &value, &ttl, &version);
    if (!s.isOK()) {
      SPDLOG_INFO("failed to read key {}: {}", key, s.getDetail());
      if (s.isNotFound()) {
        return utils::Status::invalidArg("the src keys not exist");
      } else {
        return s;
      }
    }
    (*kvsToRead)[key] = {value, version};
  }
  /// 2. run user-defined generate function
  auto s = generate(*kvsToRead,
      mContext->getRequest().entry().inputinfo(),
      mContext->getKVsToWrite(),
      mContext->getOutputInfo());
  if (!s.isOK()) {
    SPDLOG_INFO("failed to generate kv {}", s.getDetail());
    return s;
  }
  /// 3. check kv to write and lock
  auto kvsToWrite = mContext->getKVsToWrite();
  auto tgtKeys = mContext->getRequest().entry().tgtkeys();
  std::set<store::KeyType> tgtKeySet;
  std::set<store::KeyType> keySetToWrite;
  for (auto &[k, v] : *kvsToWrite) {
    keySetToWrite.insert(k);
  }
  for (auto &k : tgtKeys) {
    tgtKeySet.insert(k);
  }
  if (!std::equal(tgtKeySet.begin(), tgtKeySet.end(), keySetToWrite.begin())) {
    return utils::Status::notSupported("generated keys should be same as tgt keys");
  }
  /// acquire exclusive lock for both read/write to avoid deadlock
  return kvStore->lock(mContext->getTargetKeys(), true);
}

utils::Status KVGenerateCommand::execute(const std::shared_ptr<store::KVStore> &kvStore, EventList *events) {
  auto kvsToRead = mContext->getKVsToRead();
  auto kvsToWrite = mContext->getKVsToWrite();
  /// make sure reads are expected
  for (auto &[key, p] : *kvsToRead) {
    auto &[expectedValue, expectedVersion] = p;
    store::ValueType value;
    store::TTLType ttl;
    store::VersionType version;
    auto s = kvStore->readKV(key, &value, &ttl, &version);
    if (!s.isOK()) {
      SPDLOG_INFO("failed to read key {}: {}", key, s.getDetail());
      return s;
    }
    if (expectedVersion != version) {
      s = utils::Status::precondUnmatched(
          absl::StrCat("unmatched version, existing: ",
            std::to_string(version), ", expected: ",
            std::to_string(expectedVersion)));
      SPDLOG_INFO("check failed {}", s.getDetail());
      return s;
    }
  }

  SPDLOG_INFO("debug: executing generate cmd, read kvs count {}, write kvs count {}",
      kvsToRead->size(), kvsToWrite->size());
  for (auto &[key, value] : *kvsToWrite) {
    /// refresh ttl value if it is set before
    bool enableTTL = false;
    utils::TimeType deadline = store::NEVER_EXPIRE;
    store::TTLType ttl = store::INFINITE_TTL;
    proto::Meta meta;
    auto s = kvStore->readMeta(key, &meta);
    if (s.isOK()) {
      if (meta.ttl() != store::INFINITE_TTL && meta.deadline() > utils::TimeUtil::secondsSinceEpoch()) {
        ttl = meta.ttl();
      }
    } else if (!s.isNotFound()) {
      return s;
    }
    if (ttl != store::INFINITE_TTL) {
      enableTTL = true;
      deadline = ttl + utils::TimeUtil::secondsSinceEpoch();
    }

    SPDLOG_INFO("debug: write key {}, value {}", key, value);
    events->push_back(std::make_shared<WriteEvent>(key, value, enableTTL, ttl, deadline));
  }
  return utils::Status::ok();
}

utils::Status KVGenerateCommand::finish(const std::shared_ptr<store::KVStore> &kvStore, const EventList &events) {
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

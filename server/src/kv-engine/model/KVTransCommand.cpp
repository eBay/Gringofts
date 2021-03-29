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

#include "KVTransCommand.h"

#include <absl/strings/str_cat.h>
#include <rocksdb/db.h>
#include <spdlog/spdlog.h>

#include "../store/TemporalKVStore.h"
#include "../utils/StrUtil.h"

namespace goblin::kvengine::model {

KVTransCommandContext::KVTransCommandContext(const proto::Transaction::Request& req,
    AsyncTransCBFunc cb) : mRequest(req), mCB(cb) {
}

void KVTransCommandContext::initSuccessResponse(const store::VersionType &curMaxVersion,
    const model::EventList &events) {
  mResponse.mutable_header()->set_code(proto::ResponseCode::OK);
  std::map<store::KeyType, store::VersionType> newKey2version;
  auto latestVersion = curMaxVersion;
  for (auto &e : events) {
    auto newResult = mResponse.add_results();
    if (e->getType() == model::EventType::WRITE) {
      auto *write = dynamic_cast<const model::WriteEvent*>(e.get());
      assert(write != nullptr);
      auto allocatedVersion = write->getAllocatedVersion();
      newKey2version[write->getKey()] = allocatedVersion;
      newResult->mutable_writeresult()->set_version(allocatedVersion);
      newResult->mutable_writeresult()->set_code(proto::ResponseCode::OK);
      latestVersion = std::max(allocatedVersion, latestVersion);
    } else if (e->getType() == model::EventType::READ) {
      auto *read = dynamic_cast<const model::ReadEvent*>(e.get());
      assert(read != nullptr);
      auto key = read->getKey();
      store::VersionType valueVersion = store::VersionStore::kInvalidVersion;
      if (newKey2version.find(key) != newKey2version.end()) {
        /// we are reading the same key we newly inserted in this transaction
        valueVersion = newKey2version[key];
      } else {
        valueVersion = read->getValueVersion();
      }
      newResult->mutable_readresult()->set_value(read->getValue());
      newResult->mutable_readresult()->set_version(valueVersion);
      if (read->isNotFound()) {
        newResult->mutable_readresult()->set_code(proto::ResponseCode::KEY_NOT_EXIST);
      } else {
        newResult->mutable_readresult()->set_code(proto::ResponseCode::OK);
      }
      latestVersion = std::max(valueVersion, latestVersion);
    } else if (e->getType() == model::EventType::DELETE) {
      /// TODO: support delete in transaction
      /*
      auto *deleteEvent = dynamic_cast<const model::DeleteEvent*>(e.get());
      assert(deleteEvent != nullptr);
      newResult->mutable_deleteresult()->set_value(deleteEvent->getDeletedValue());
      newResult->mutable_writeresult()->set_version(deleteEvent->getDeletedVersion());
      newResult->mutable_writeresult()->set_code(proto::ResponseCode::OK);
      auto allocatedVersion = deleteEvent.getAllocatedVersion();
      latestVersion = std::max(allocatedVersion, latestVersion);
      */
      assert(0);
    } else {
      assert(0);
    }
  }
  mResponse.mutable_header()->set_latestversion(latestVersion);
}

void KVTransCommandContext::fillResponseAndReply(proto::ResponseCode code,
    const std::string &message, std::optional<uint64_t> leaderId) {
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
    for (auto &r : *mResponse.mutable_results()) {
      if (r.has_writeresult()) {
        r.mutable_writeresult()->set_code(code);
      } else if (r.has_readresult()) {
        r.mutable_readresult()->set_code(code);
      } else {
        assert(0);
      }
    }
  }
  mResponse.mutable_header()->set_message(message);
  mCB(mResponse);
}

void KVTransCommandContext::reportSubMetrics() {
  /// metrics counter
  auto transCounter = gringofts::getCounter("trans_command_counter", {});
  transCounter.increase();
  /// SPDLOG_INFO("debug: trans op");
}

const proto::Transaction::Request& KVTransCommandContext::getRequest() {
  return mRequest;
}

KVTransCommand::KVTransCommand(std::shared_ptr<KVTransCommandContext> context):
  mContext(context) {
}

std::shared_ptr<CommandContext> KVTransCommand::getContext() {
  return mContext;
}

std::set<store::KeyType> KVTransCommandContext::getTargetKeys() {
  std::set<store::KeyType> keys;
  for (auto &cond : mRequest.preconds()) {
    if (cond.has_versioncond()) {
      keys.insert(cond.versioncond().key());
    } else if (cond.has_existcond()) {
      keys.insert(cond.existcond().key());
    }
  }
  for (auto readOrWrite : mRequest.entries()) {
    if (readOrWrite.has_writeentry()) {
      const store::KeyType &key = readOrWrite.writeentry().key();
      keys.insert(key);
    } else if (readOrWrite.has_readentry()) {
      const store::KeyType &key = readOrWrite.readentry().key();
      keys.insert(key);
    } else {
      /// TODO: support delete in transaction
    }
  }
  return keys;
}

proto::RequestHeader KVTransCommandContext::getRequestHeader() {
  return mRequest.header();
}

utils::Status KVTransCommand::prepare(const std::shared_ptr<store::KVStore> &kvStore) {
  /// acquire exclusive lock
  return kvStore->lock(mContext->getTargetKeys(), true);
}

utils::Status KVTransCommand::checkPrecond(std::shared_ptr<store::KVStore> kvStore) {
  auto &req = mContext->getRequest();
  utils::Status s = utils::Status::ok();
  for (auto &cond : req.preconds()) {
    if (cond.has_versioncond()) {
      auto key = cond.versioncond().key();
      auto expectedVersion = cond.versioncond().version();
      auto op = cond.versioncond().op();
      store::ValueType value;
      store::TTLType ttl = store::INFINITE_TTL;
      store::VersionType version = store::VersionStore::kInvalidVersion;
      s = kvStore->readKV(key, &value, &ttl, &version);
      SPDLOG_INFO("debug: precond, key {}, value {}, newVersion {}", key, value, version);
      if (!s.isOK()) {
        s = utils::Status::precondUnmatched(s.getDetail());
        SPDLOG_INFO("check failed {}", s.getDetail());
        break;
      }
      if (op == proto::Precondition::EQUAL) {
        if (expectedVersion != version) {
          s = utils::Status::precondUnmatched(
              absl::StrCat("unmatched version, existing: ",
                std::to_string(version), ", expected: ",
                std::to_string(expectedVersion)));
          SPDLOG_INFO("check failed {}", s.getDetail());
          break;
        }
      } else {
        s = utils::Status::notSupported();
        break;
      }
    } else if (cond.has_existcond()) {
      auto key = cond.existcond().key();
      auto shouldExist = cond.existcond().shouldexist();
      store::ValueType value;
      store::TTLType ttl;
      store::VersionType version;
      s = kvStore->readKV(key, &value, &ttl, &version);
      SPDLOG_INFO("debug: shouldexist {}, key {}, result {}", shouldExist, key, s.getDetail());
      if (shouldExist && s.isOK()) {
        s = utils::Status::ok();
      } else if (!shouldExist && s.isNotFound()) {
        s = utils::Status::ok();
      } else {
        s = utils::Status::precondUnmatched(
            absl::StrCat("shouldExist: ",
              std::to_string(shouldExist), ", query result: ",
              s.getDetail()));
        break;
      }
    } else {
      s = utils::Status::notSupported();
      break;
    }
  }
  return s;
}

utils::Status KVTransCommand::execute(const std::shared_ptr<store::KVStore> &kvStore, EventList *events) {
  store::TemporalKVStore tempStore(std::make_shared<store::ReadOnlyKVStore>(kvStore));
  auto s = checkPrecond(kvStore);
  auto &req = mContext->getRequest();
  do {
    if (!s.isOK()) {
      break;
    }
    for (auto readOrWrite : req.entries()) {
      if (readOrWrite.has_writeentry()) {
        const store::KeyType &key = readOrWrite.writeentry().key();
        const store::ValueType &value = readOrWrite.writeentry().value();
        bool enableTTL = readOrWrite.writeentry().enablettl();
        store::TTLType ttl = enableTTL? readOrWrite.writeentry().ttl() : store::INFINITE_TTL;
        /// fix ttl value if not set while ttl enabled
        if (enableTTL && ttl == store::INFINITE_TTL) {
          proto::Meta meta;
          s = kvStore->readMeta(key, &meta);
          if (s.isOK()) {
            if (meta.ttl() != store::INFINITE_TTL && meta.deadline() > utils::TimeUtil::secondsSinceEpoch()) {
              ttl = meta.ttl();
            } else {
              enableTTL = false;
            }
          } else if (s.isNotFound()) {
            /// Keep the value forever if previous ttl not found or value is expired
            enableTTL = false;
          } else {
            break;
          }
        }

        const utils::TimeType &deadline = enableTTL? ttl + utils::TimeUtil::secondsSinceEpoch() : store::NEVER_EXPIRE;
        if (enableTTL) {
          s = tempStore.writeTTLKV(key, value, store::VersionStore::kInvalidVersion, ttl, deadline);
        } else {
          s = tempStore.writeKV(key, value, store::VersionStore::kInvalidVersion);
        }
        SPDLOG_INFO("debug: executing put cmd, key {}, value {}", key, value);
        events->push_back(std::make_shared<WriteEvent>(key, value, enableTTL, ttl, deadline));
      } else if (readOrWrite.has_readentry()) {
        const store::KeyType &key = readOrWrite.readentry().key();
        store::ValueType value;
        store::TTLType ttl;
        store::VersionType version;
        /// the value we read may not be committed at this moment
        /// but the ReadEvent generated will wait until it it commited and reply to clients
        s = tempStore.readKV(key, &value, &ttl, &version);
        SPDLOG_INFO("debug: executing get cmd, key {}, value {}, newVersion {}", key, value, version);
        if (s.isNotFound()) {
          events->push_back(std::make_shared<ReadEvent>(key, value, ttl, version, true));
        } else if (s.isOK()) {
          events->push_back(std::make_shared<ReadEvent>(key, value, ttl, version, false));
        } else {
          break;
        }
      } else {
        /// TODO: support delete in transaction
        s = utils::Status::invalidArg("invalid request");
        break;
      }
    }
  } while (0);
  return s;
}

utils::Status KVTransCommand::finish(const std::shared_ptr<store::KVStore> &kvStore, const EventList &events) {
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

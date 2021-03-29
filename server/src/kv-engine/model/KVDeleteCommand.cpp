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

#include "KVDeleteCommand.h"

#include "../utils/StrUtil.h"

namespace goblin::kvengine::model {

KVDeleteCommandContext::KVDeleteCommandContext(
    const proto::Delete::Request& req,
    AsyncDeleteCBFunc cb) : mRequest(req), mCB(cb) {
}

void KVDeleteCommandContext::initSuccessResponse(
    const store::VersionType &curMaxVersion,
    const model::EventList &events) {
  mResponse.mutable_result()->set_code(proto::ResponseCode::OK);
  mResponse.mutable_header()->set_code(proto::ResponseCode::OK);
  auto latestVersion = curMaxVersion;
  if (events.size() > 0) {
    assert(events.size() == 1);
    auto &e = events[0];
    if (e->getType() == model::EventType::DELETE) {
      auto *deleteEvent = dynamic_cast<const model::DeleteEvent*>(e.get());
      assert(deleteEvent != nullptr);
      mResponse.mutable_result()->set_value(deleteEvent->getDeletedValue());
      mResponse.mutable_result()->set_version(deleteEvent->getDeletedVersion());
      auto allocatedVersion = deleteEvent->getAllocatedVersion();
      latestVersion = std::max(allocatedVersion, curMaxVersion);
    } else if (e->getType() == model::EventType::READ) {
      auto *readEvent = dynamic_cast<const model::ReadEvent*>(e.get());
      assert(readEvent != nullptr);
      assert(readEvent->isNotFound());
      mResponse.mutable_result()->set_code(proto::ResponseCode::KEY_NOT_EXIST);
      latestVersion = curMaxVersion;
    } else {
      assert(0);
    }
  }
  mResponse.mutable_header()->set_latestversion(latestVersion);
}

void KVDeleteCommandContext::fillResponseAndReply(
    proto::ResponseCode code,
    const std::string &message,
    std::optional<uint64_t> leaderId) {
  if (code == proto::ResponseCode::NOT_LEADER && leaderId) {
    mResponse.mutable_header()->set_leaderhint(std::to_string(*leaderId));
  } else if (code == proto::ResponseCode::MIGRATED_ROUTE) {
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

void KVDeleteCommandContext::reportSubMetrics() {
  /// metrics counter
  auto deleteCounter = gringofts::getCounter("delete_command_counter", {});
  deleteCounter.increase();
  /// SPDLOG_INFO("debug: delete op");
}

const proto::Delete::Request& KVDeleteCommandContext::getRequest() {
  return mRequest;
}


KVDeleteCommand::KVDeleteCommand(std::shared_ptr<KVDeleteCommandContext> context):
  mContext(context) {
}

std::shared_ptr<CommandContext> KVDeleteCommand::getContext() {
  return mContext;
}

std::set<store::KeyType> KVDeleteCommandContext::getTargetKeys() {
  return {mRequest.entry().key()};
}

proto::RequestHeader KVDeleteCommandContext::getRequestHeader() {
  return mRequest.header();
}

utils::Status KVDeleteCommand::prepare(const std::shared_ptr<store::KVStore> &kvStore) {
  /// acquire exclusive lock
  return kvStore->lock(mContext->getTargetKeys(), true);
}

utils::Status KVDeleteCommand::execute(const std::shared_ptr<store::KVStore> &kvStore, EventList *events) {
  auto &req = mContext->getRequest();
  utils::Status s = utils::Status::ok();
  do {
    bool returnValue = req.returnvalue();
    const store::KeyType &key = req.entry().key();
    uint64_t targetVersion = req.entry().version();
    store::ValueType deletedValue;
    store::TTLType deletedTTL;
    store::VersionType deletedVersion;
    s = kvStore->readKV(key, &deletedValue, &deletedTTL, &deletedVersion);
    SPDLOG_INFO("debug: executing delete cmd, key {}, target version {}, found version {}",
        key, targetVersion, deletedVersion);
    if (s.isNotFound()) {
      events->push_back(std::make_shared<ReadEvent>(key, deletedValue, deletedTTL, deletedVersion, true));
    } else if (s.isOK()) {
      if (targetVersion != store::VersionStore::kInvalidVersion && targetVersion != deletedVersion) {
        events->push_back(std::make_shared<ReadEvent>(key, deletedValue, deletedTTL, deletedVersion, true));
      } else {
        if (!returnValue) {
          deletedValue = "";
        }
        events->push_back(std::make_shared<DeleteEvent>(key, deletedValue, deletedVersion));
      }
    }
  } while (0);
  return s;
}

utils::Status KVDeleteCommand::finish(const std::shared_ptr<store::KVStore> &kvStore, const EventList &events) {
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

}  // namespace goblin::kvengine::model

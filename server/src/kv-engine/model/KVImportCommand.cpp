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

#include "KVImportCommand.h"

namespace goblin::kvengine::model {

KVImportCommand::KVImportCommand(std::shared_ptr<KVPutCommandContext> context):
  mContext(context) {
}

std::shared_ptr<CommandContext> KVImportCommand::getContext() {
  return mContext;
}

utils::Status KVImportCommand::prepare(const std::shared_ptr<store::KVStore> &kvStore) {
  /// acquire exclusive lock
  return kvStore->lock(mContext->getTargetKeys(), true);
}

utils::Status KVImportCommand::execute(const std::shared_ptr<store::KVStore> &kvStore, EventList *events) {
  using store::INFINITE_TTL;
  using store::NEVER_EXPIRE;
  using utils::TimeUtil;

  auto &req = mContext->getRequest();
  const store::KeyType &key = req.entry().key();
  const store::ValueType &value = req.entry().value();
  const store::VersionType &importedVersion = req.entry().version();

  bool enableTTL = req.entry().enablettl();
  store::TTLType ttl = enableTTL? req.entry().ttl() : INFINITE_TTL;

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

  /// SPDLOG_INFO("debug: executing put cmd, key {}, value {}, ttl {}, enableTTL {}, deadline {}",
  /// key, value, ttl, enableTTL, deadline);
  events->push_back(std::make_shared<ImportEvent>(importedVersion, key, value, enableTTL, ttl, deadline));
  return utils::Status::ok();
}

utils::Status KVImportCommand::finish(const std::shared_ptr<store::KVStore> &kvStore, const EventList &events) {
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

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
#ifndef SERVER_SRC_KV_ENGINE_MODEL_KVEVICTCOMMAND_H_
#define SERVER_SRC_KV_ENGINE_MODEL_KVEVICTCOMMAND_H_

#include "Command.h"
#include "../store/KVStore.h"

namespace goblin::kvengine::model {
class KVEvictCommand : public Command {
 public:
  KVEvictCommand(const std::set<store::KeyType> &keys,
                 const store::VersionType &version)
      : mEvictKeys(keys), mEvictVersion(version) {}

  utils::Status prepare(const std::shared_ptr<store::KVStore> &) override;
  utils::Status execute(const std::shared_ptr<store::KVStore> &, EventList *) override;
  utils::Status finish(const std::shared_ptr<store::KVStore> &kvStore, const EventList &events) override;

  virtual ~KVEvictCommand() = default;

 private:
  std::set<store::KeyType> mEvictKeys;
  store::VersionType mEvictVersion;
};
}  // namespace goblin::kvengine::model
#endif  // SERVER_SRC_KV_ENGINE_MODEL_KVEVICTCOMMAND_H_

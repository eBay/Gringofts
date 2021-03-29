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

#ifndef SERVER_SRC_KV_ENGINE_MODEL_KVDELETECOMMAND_H_
#define SERVER_SRC_KV_ENGINE_MODEL_KVDELETECOMMAND_H_

#include "Command.h"

namespace goblin::kvengine::model {

using AsyncDeleteCBFunc = std::function<void(const proto::Delete::Response&)>;

class KVDeleteCommandContext : public CommandContext {
 public:
  KVDeleteCommandContext(const proto::Delete::Request&, AsyncDeleteCBFunc);
  void initSuccessResponse(const store::VersionType &curMaxVersion, const model::EventList &events) override;
  void fillResponseAndReply(
      proto::ResponseCode code, const std::string &message, std::optional<uint64_t> leaderId) override;

  void reportSubMetrics() override;

  const proto::Delete::Request& getRequest();

  std::set<store::KeyType> getTargetKeys() override;
  proto::RequestHeader getRequestHeader() override;
  bool skipPreExecuteCB() override {
     return false;
  }
 private:
  proto::Delete::Request mRequest;
  proto::Delete::Response mResponse;
  AsyncDeleteCBFunc mCB;
};


class KVDeleteCommand final : public Command {
 public:
  explicit KVDeleteCommand(std::shared_ptr<KVDeleteCommandContext> context);

  std::shared_ptr<CommandContext> getContext() override;

  utils::Status prepare(const std::shared_ptr<store::KVStore> &) override;
  utils::Status execute(const std::shared_ptr<store::KVStore> &, EventList *) override;
  utils::Status finish(const std::shared_ptr<store::KVStore> &kvStore, const EventList &events) override;

 private:
  std::shared_ptr<KVDeleteCommandContext> mContext;
};

}  // namespace goblin::kvengine::model

#endif  // SERVER_SRC_KV_ENGINE_MODEL_KVDELETECOMMAND_H_

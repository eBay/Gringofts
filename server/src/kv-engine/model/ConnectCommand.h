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

#ifndef SERVER_SRC_KV_ENGINE_MODEL_CONNECTCOMMAND_H_
#define SERVER_SRC_KV_ENGINE_MODEL_CONNECTCOMMAND_H_

#include "Command.h"

namespace goblin::kvengine::model {

using AsyncConnectCBFunc = std::function<void(const proto::Connect::Response&)>;

class ConnectCommandContext : public CommandContext {
 public:
  ConnectCommandContext(const proto::Connect::Request&, AsyncConnectCBFunc);
  void initSuccessResponse(const store::VersionType &curMaxVersion, const model::EventList &events) override;
  void fillResponseAndReply(
      proto::ResponseCode code,
      const std::string &message,
      std::optional<uint64_t> leaderId) override;
  void reportSubMetrics() override;

  const proto::Connect::Request& getRequest();

  std::set<store::KeyType> getTargetKeys() override;
  proto::RequestHeader getRequestHeader() override;
  bool skipPreExecuteCB() override {
     return false;
  }
 private:
  proto::Connect::Request mRequest;
  proto::Connect::Response mResponse;
  AsyncConnectCBFunc mCB;
};

class ConnectCommand final : public Command {
 public:
  explicit ConnectCommand(std::shared_ptr<ConnectCommandContext> context);

  std::shared_ptr<CommandContext> getContext() override;

  utils::Status prepare(const std::shared_ptr<store::KVStore> &) override;
  utils::Status execute(const std::shared_ptr<store::KVStore> &, EventList *) override;
  utils::Status finish(const std::shared_ptr<store::KVStore> &kvStore, const EventList &events) override;

 private:
  std::shared_ptr<ConnectCommandContext> mContext;
};

}  // namespace goblin::kvengine::model

#endif  // SERVER_SRC_KV_ENGINE_MODEL_CONNECTCOMMAND_H_

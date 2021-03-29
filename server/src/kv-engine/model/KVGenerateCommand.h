/************************************************************************
Copyright 2020-2021 eBay Inc.
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

#ifndef SERVER_SRC_KV_ENGINE_MODEL_KVGENERATECOMMAND_H_
#define SERVER_SRC_KV_ENGINE_MODEL_KVGENERATECOMMAND_H_

#include "Command.h"

namespace goblin::kvengine::model {

using AsyncGenerateCBFunc = std::function<void(const proto::GenerateKV::Response&)>;
using InputInfoType = std::string;
using OutputInfoType = std::string;

class KVGenerateCommandContext : public CommandContext {
 public:
  KVGenerateCommandContext(const proto::GenerateKV::Request&, AsyncGenerateCBFunc);

  void initSuccessResponse(const store::VersionType &curMaxVersion, const model::EventList &events) override;
  void fillResponseAndReply(proto::ResponseCode code,
      const std::string &message, std::optional<uint64_t> leaderId) override;

  void reportSubMetrics() override;

  const proto::GenerateKV::Request& getRequest();
  std::map<store::KeyType, std::pair<store::ValueType, store::VersionType>> *getKVsToRead();
  std::map<store::KeyType, store::ValueType> *getKVsToWrite();
  OutputInfoType *getOutputInfo();

  std::set<store::KeyType> getTargetKeys() override;
  proto::RequestHeader getRequestHeader() override;
  bool skipPreExecuteCB() override {
     return false;
  }

 private:
  proto::GenerateKV::Request mRequest;
  proto::GenerateKV::Response mResponse;
  std::map<store::KeyType, std::pair<store::ValueType, store::VersionType>> mKVsToRead;
  std::map<store::KeyType, store::ValueType> mKVsToWrite;
  OutputInfoType mOutputInfo;
  AsyncGenerateCBFunc mCB;
};

class KVGenerateCommand : public Command {
 public:
  explicit KVGenerateCommand(std::shared_ptr<KVGenerateCommandContext> context);
  virtual ~KVGenerateCommand() = default;

  std::shared_ptr<CommandContext> getContext() override;

  utils::Status prepare(const std::shared_ptr<store::KVStore> &) override;
  utils::Status execute(const std::shared_ptr<store::KVStore> &, EventList *) override;
  virtual utils::Status generate(
      const std::map<store::KeyType, std::pair<store::ValueType, store::VersionType>> &kvsToRead,
      const InputInfoType &inputInfo,
      std::map<store::KeyType, store::ValueType> *kvsToWrite,
      OutputInfoType *outputInfo) = 0;
  utils::Status finish(const std::shared_ptr<store::KVStore> &kvStore, const EventList &events) override;

 private:
  std::shared_ptr<KVGenerateCommandContext> mContext;
};

}  // namespace goblin::kvengine::model

#endif  // SERVER_SRC_KV_ENGINE_MODEL_KVGENERATECOMMAND_H_

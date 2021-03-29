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

#ifndef SERVER_SRC_KV_ENGINE_EXECUTION_COMMANDPROCESSOR_H_
#define SERVER_SRC_KV_ENGINE_EXECUTION_COMMANDPROCESSOR_H_
#include <infra/mpscqueue/MpscQueue.h>

#include <utility>

#include "../store/ProxyKVStore.h"
#include "../model/Command.h"
#include "../raft/RaftEventStore.h"
#include "../types.h"
#include "QueueWorker.h"

namespace goblin::kvengine::execution {
class CommandProcessor : public Processor<std::shared_ptr<model::Command>> {
 public:
  typedef std::shared_ptr<store::VersionStore> VersionStorePtr;
  typedef std::shared_ptr<store::ProxyKVStore> KVStorePtr;
  typedef std::shared_ptr<raft::RaftEventStore> RaftStorePtr;
  CommandProcessor(
       VersionStorePtr versionStore,
       KVStorePtr kvStore,
       RaftStorePtr raft,
       BecomeLeaderCallBack becomeLeaderCB = nullptr,
       PreExecuteCallBack preExecuteCB = nullptr)
     : mVersionStorePtr(std::move(versionStore)),
     mKVStorePtr(std::move(kvStore)),
     mRaftStorePtr(std::move(raft)),
     mBecomeLeaderCallBack(becomeLeaderCB),
     mPreExecuteCallBack(preExecuteCB) {}
  CommandProcessor(const CommandProcessor &) = delete;

 protected:
  void process(const std::shared_ptr<model::Command> &) override;

 private:
  void doExecute(const std::shared_ptr<model::Command> &command);

  VersionStorePtr mVersionStorePtr;
  KVStorePtr mKVStorePtr;
  RaftStorePtr mRaftStorePtr;
  BecomeLeaderCallBack mBecomeLeaderCallBack;
  PreExecuteCallBack mPreExecuteCallBack;
  /// current term this processor is working on
  /// when term is changed, we need to clean VersionStore and KVStore
  std::atomic<uint64_t> mCurTerm = 0;

  std::mutex mBecomeLeaderMutex;
  std::mutex mProcessComamndMutex;
  std::condition_variable mProcessCommandCv;
  std::atomic<uint64_t> mRunningCmdCnt = 0;
};
}  // namespace goblin::kvengine::execution
#endif  // SERVER_SRC_KV_ENGINE_EXECUTION_COMMANDPROCESSOR_H_

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

#ifndef SERVER_SRC_KV_ENGINE_KVENGINEIMPL_H_
#define SERVER_SRC_KV_ENGINE_KVENGINEIMPL_H_

#include <infra/util/CryptoUtil.h>
#include <infra/util/TestPointProcessor.h>

#include "execution/CommandProcessor.h"
#include "execution/EventApplyLoop.h"
#include "execution/ExecutionService.h"
#include "model/Command.h"
#include "strategy/CacheEviction.h"
#include "types.h"
#include "utils/Status.h"

namespace goblin {
namespace mock {
template <typename ClusterType>
class MockAppCluster;
}
}

namespace goblin::kvengine {

class KVEngineImpl final {
 public:
  using CommandPtr = std::shared_ptr<model::Command>;
  template<class T> using ExecutionServicePtr = std::shared_ptr<execution::ExecutionService<T>>;

  utils::Status Init(const char *configPath,
       const std::vector<store::WSName> &initWSNames,
       store::WSLookupFunc wsLookupFunc,
       execution::BecomeLeaderCallBack becomeLeaderCallBack = nullptr,
       execution::PreExecuteCallBack preExecuteCallBack = nullptr,
       const std::shared_ptr<store::KVObserver>& = nullptr);
  utils::Status Destroy();
  ~KVEngineImpl();

  proto::Connect::Response connect(const proto::Connect::Request&);
  proto::Put::Response put(const proto::Put::Request&);
  proto::Get::Response get(const proto::Get::Request&);
  proto::Delete::Response remove(const proto::Delete::Request&);
  proto::ExeBatch::Response exeBatch(const proto::ExeBatch::Request&);
  proto::Transaction::Response trans(const proto::Transaction::Request&);
  proto::ExeBatch::Response getAppliedBatch(const proto::ExeBatch::Request &);

  utils::Status connectAsync(
      const proto::Connect::Request&,
      std::function<void(const proto::Connect::Response&)> cb);
  utils::Status putAsync(
      const proto::Put::Request&,
      std::function<void(const proto::Put::Response&)> cb);
  utils::Status getAsync(
      const proto::Get::Request&,
      std::function<void(const proto::Get::Response&)> cb);
  utils::Status removeAsync(
      const proto::Delete::Request&,
      std::function<void(const proto::Delete::Response&)> cb);
  utils::Status exeBatchAsync(
      const proto::ExeBatch::Request&,
      std::function<void(const proto::ExeBatch::Response&)> cb);
  utils::Status transAsync(
      const proto::Transaction::Request&,
      std::function<void(const proto::Transaction::Response&)> cb);
  utils::Status migrateBatchAsync(
      const proto::MigrateBatch::Request&,
      std::function<void(const proto::MigrateBatch::Response&)> cb);
  /// execute a customized command
  utils::Status exeCustomCommand(std::shared_ptr<model::Command> cmd);

 private:
  void startProcessCommandLoop(
       const INIReader &reader,
       const std::vector<store::WSName> &initWSNames,
       store::WSLookupFunc wsLookupFunc,
       execution::BecomeLeaderCallBack becomeLeaderCallBack = nullptr,
       execution::PreExecuteCallBack preExecuteCallBack = nullptr);

  void startEventApplyLoop();

  void initMonitor(const INIReader &reader);
  /// migrate command can only used by the server itself
  utils::Status importAsync(const proto::Put::Request &req, std::function<void(const proto::Put::Response&)> cb);

  /*** the order of the following declaration matters ***/
  std::shared_ptr<gringofts::CryptoUtil> mCrypto;
  std::shared_ptr<gringofts::raft::RaftInterface> mRaftImpl;

  std::shared_ptr<store::VersionStore> mVersionStore;
  std::shared_ptr<store::KVStore> mRocksDBKVStore;

  std::shared_ptr<raft::ReplyLoop> mReplyLoop;
  std::shared_ptr<raft::RaftEventStore> mRaftEventStore;
  std::shared_ptr<execution::EventApplyLoop> mEventApplyLoop;

  ExecutionServicePtr<CommandPtr> mCommandProcessExecutionService;

  std::shared_ptr<strategy::CacheEviction> mCacheEviction;

  std::thread mEventApplyLoopThread;

  /// UT
  utils::Status InitForTest(
       const char *configPath,
       const std::vector<store::WSName> &initWSNames,
       store::WSLookupFunc wsLookupFunc,
       execution::BecomeLeaderCallBack becomeLeaderCallBack,
       execution::PreExecuteCallBack preExecuteCallBack,
       std::shared_ptr<gringofts::raft::RaftInterface> raftInst,
       std::shared_ptr<store::VersionStore> versionStorePtr,
       std::shared_ptr<store::RocksDBKVStore> rocksdbPtr,
       std::shared_ptr<raft::ReplyLoop> replyLoopPtr,
       std::shared_ptr<raft::RaftEventStore> eventStorePtr,
       std::shared_ptr<execution::EventApplyLoop> eventApplyPtr,
       gringofts::TestPointProcessor *processor);
  gringofts::TestPointProcessor *mTPProcessor = nullptr;
  template<typename T>
  friend class mock::MockAppCluster;
};

}  /// namespace goblin::kvengine

#endif  // SERVER_SRC_KV_ENGINE_KVENGINEIMPL_H_

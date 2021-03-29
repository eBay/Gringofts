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

#ifndef SERVER_SRC_KV_ENGINE_KVENGINE_H_
#define SERVER_SRC_KV_ENGINE_KVENGINE_H_

#include <functional>

#include "../../../protocols/generated/service.pb.h"
#include "types.h"
#include "utils/Status.h"
#include "store/KVStore.h"

namespace goblin {
namespace mock {
template <typename ClusterType>
class MockAppCluster;
}
}

namespace goblin::kvengine {

class KVEngineImpl;

class KVEngine final {
 public:
  utils::Status Init(
       const char *configPath,
       const std::vector<store::WSName> &initWSNames,
       store::WSLookupFunc wsLookupFunc,
       execution::BecomeLeaderCallBack becomeLeaderCallBack = nullptr,
       execution::PreExecuteCallBack preExecuteCallBack = nullptr,
       const std::shared_ptr<store::KVObserver>& = nullptr);
  utils::Status Destroy();

  proto::Connect::Response connect(const proto::Connect::Request&);
  proto::Put::Response put(const proto::Put::Request&);
  proto::Get::Response get(const proto::Get::Request&);
  proto::Delete::Response remove(const proto::Delete::Request&);
  proto::ExeBatch::Response exeBatch(const proto::ExeBatch::Request&);
  proto::Transaction::Response trans(const proto::Transaction::Request&);
  proto::ExeBatch::Response getAppliedBatch(const proto::ExeBatch::Request &);

  utils::Status connectAsync(const proto::Connect::Request&,
      std::function<void(const proto::Connect::Response&)> cb);
  utils::Status putAsync(const proto::Put::Request&,
      std::function<void(const proto::Put::Response&)> cb);
  utils::Status getAsync(const proto::Get::Request&,
      std::function<void(const proto::Get::Response&)> cb);
  utils::Status removeAsync(const proto::Delete::Request&,
      std::function<void(const proto::Delete::Response&)> cb);
  utils::Status exeBatchAsync(const proto::ExeBatch::Request&,
      std::function<void(const proto::ExeBatch::Response&)> cb);
  utils::Status transAsync(const proto::Transaction::Request&,
      std::function<void(const proto::Transaction::Response&)> cb);
  utils::Status migrateBatchAsync(const proto::MigrateBatch::Request&,
      std::function<void(const proto::MigrateBatch::Response&)> cb);
  /// execute a customized command
  utils::Status exeCustomCommand(std::shared_ptr<model::Command> cmd);

 private:
  std::shared_ptr<KVEngineImpl> mImpl;

  /// UT
  utils::Status InitForTest(std::shared_ptr<KVEngineImpl> impl);
  template <typename ClusterType>
  friend class mock::MockAppCluster;
};

}  /// namespace goblin::kvengine

#endif  // SERVER_SRC_KV_ENGINE_KVENGINE_H_

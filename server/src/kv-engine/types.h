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

#ifndef SERVER_SRC_KV_ENGINE_TYPES_H_
#define SERVER_SRC_KV_ENGINE_TYPES_H_

#include <vector>

#include <infra/mpscqueue/MpscQueue.h>
#include <infra/raft/RaftInterface.h>

#include "utils/Status.h"

namespace goblin::kvengine {

namespace store {
class KVStore;

  using KeyType = std::string;
  using ValueType = std::string;
  using VersionType = uint64_t;
  using MilestoneType = uint64_t;
  /// TTL measured in second, value is seconds since epoch(1 Jan 1970), 32-bit unsigned int is enough
  using TTLType = uint32_t;

  using WSName = std::string;
  using WSLookupFunc = std::function<WSName(const store::KeyType &)>;
}  // namespace store

namespace model {
class CommandContext;
class Command;
}

namespace execution {
  using BecomeLeaderCallBack = std::function<utils::Status(
      const std::vector<gringofts::raft::MemberInfo> & /* cluster info */)>;
  using PreExecuteCallBack = std::function<utils::Status(
      model::CommandContext &context,
      store::KVStore &kvStore)>;
}

template<class T>
using BlockingQueue = gringofts::MpscQueue<T>;

}  // namespace goblin::kvengine
#endif  // SERVER_SRC_KV_ENGINE_TYPES_H_

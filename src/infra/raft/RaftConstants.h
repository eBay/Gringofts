/************************************************************************
Copyright 2019-2020 eBay Inc.
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

#ifndef SRC_INFRA_RAFT_RAFTCONSTANTS_H_
#define SRC_INFRA_RAFT_RAFTCONSTANTS_H_

#include <cstdint>

namespace gringofts {
namespace raft {

struct RaftConstants {
  /// The minimum/maximum timeout follower will wait before starting a new election
  static constexpr uint64_t kMinElectionTimeoutInMillis = 1000;
  static constexpr uint64_t kMaxElectionTimeoutInMillis = kMinElectionTimeoutInMillis * 2;

  /// heart beat interval that leader will wait before sending a heartbeat to follower
  static const uint64_t kHeartBeatIntervalInMillis = 20;

  struct AppendEntries { static constexpr uint64_t kRpcTimeoutInMillis = 300; };
  struct RequestVote   { static constexpr uint64_t kRpcTimeoutInMillis = 100; };
};

}  /// namespace raft
}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_RAFTCONSTANTS_H_

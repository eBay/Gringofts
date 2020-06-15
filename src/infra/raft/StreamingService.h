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

#ifndef SRC_INFRA_RAFT_STREAMINGSERVICE_H_
#define SRC_INFRA_RAFT_STREAMINGSERVICE_H_

#include <atomic>

#include <INIReader.h>

#include "../../infra/monitor/MonitorTypes.h"
#include "generated/streaming.grpc.pb.h"
#include "RaftInterface.h"

namespace gringofts {
namespace raft {

class StreamingService : public Streaming::Service {
 public:
  StreamingService(const INIReader &, const RaftInterface &raftImpl);
  ~StreamingService() override;

  grpc::Status GetMeta(grpc::ServerContext *context,
                       const GetMeta::Request *request,
                       GetMeta::Response *response) override;

  grpc::Status GetEntries(grpc::ServerContext *context,
                          const GetEntries::Request *request,
                          GetEntries::Response *response) override;

 private:
  static GetMeta_Role resolveRole(raft::RaftRole role) {
    switch (role) {
      case raft::RaftRole::Leader: return GetMeta_Role_LEADER;
      case raft::RaftRole::Follower: return GetMeta_Role_FOLLOWER;
      case raft::RaftRole::Candidate: return GetMeta_Role_CANDIDATE;
      default: return GetMeta_Role_UNKNOWN_ROLE;
    }
  }

  grpc::Status getEntries(grpc::ServerContext *context,
                          const GetEntries::Request *request,
                          GetEntries::Response *response);

  const RaftInterface &mRaftImpl;

  std::unique_ptr<grpc::Server> mServer;

  /// throttle
  int64_t mMaxConcurrency = 10;
  std::atomic<uint64_t> mCurrentConcurrency = 0;

  /// metrics
  mutable santiago::MetricsCenter::CounterType mStreamingServiceGetEntriesInvokingNumber;
};

}   /// namespace raft
}   /// namespace gringofts

#endif  // SRC_INFRA_RAFT_STREAMINGSERVICE_H_

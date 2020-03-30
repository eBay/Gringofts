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

#include <INIReader.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include "../raft/RaftBuilder.h"
#include "../raft/RaftInterface.h"
#include "../raft/RaftLogStore.h"
#include "../util/TlsUtil.h"
#include "generated/interop.grpc.pb.h"

namespace gringofts {
namespace app {

/// grpc-related
using ::grpc::Server;
using ::grpc::ServerContext;
using ::grpc::ServerBuilder;
using ::grpc::Status;
using ::grpc::Channel;
using ::grpc::ClientContext;

using gringofts::interop::InteropReadMetaService;
using gringofts::interop::ReadMeta_Request;
using gringofts::interop::ReadMeta_Response;
using gringofts::interop::ReadMeta_Role;

class Sidecar final : public InteropReadMetaService::Service {
 public:
  explicit Sidecar(const char *configPath) {
    INIReader reader(configPath);

    mIpPort = reader.Get("sidecar", "ip.port", "UNKNOWN");
    assert(mIpPort != "UNKNOWN");

    mTlsConfOpt = TlsUtil::parseTlsConf(reader, "tls");

    const auto &raftConfigPath = reader.Get("store", "raft.config.path", "UNKNOWN");
    assert(raftConfigPath != "UNKNOWN");

    mRaftImpl = raft::buildRaftImpl(raftConfigPath.c_str());
    auto crypto = std::make_shared<CryptoUtil>();
    crypto->init(reader);
  }

  ~Sidecar() = default;

  /// disallow copy/move ctor/assignment
  Sidecar(const Sidecar &) = delete;
  Sidecar &operator=(const Sidecar &) = delete;

  Status ReadMeta(ServerContext *context,
                  const ReadMeta_Request *request,
                  ReadMeta_Response *reply) override {
    reply->set_code(0);
    reply->set_message("success");

    /// populate meta
    auto role = resolveRole(mRaftImpl->getRaftRole());
    auto term = mRaftImpl->getCurrentTerm();
    auto lastIndex = mRaftImpl->getLastLogIndex();
    auto commitIndex = mRaftImpl->getCommitIndex();
    auto leaderHint = mRaftImpl->getLeaderHint();

    reply->set_role(role);
    reply->set_term(term);
    reply->set_last_index(lastIndex);
    reply->set_commit_index(commitIndex);
    reply->set_leader_hint(std::to_string(*leaderHint));

    return Status::OK;
  }

  static ReadMeta_Role resolveRole(raft::RaftRole role) {
    switch (role) {
      case raft::RaftRole::Leader: return interop::ReadMeta_Role_LEADER;
      case raft::RaftRole::Follower: return interop::ReadMeta_Role_FOLLOWER;
      case raft::RaftRole::Candidate: return interop::ReadMeta_Role_CANDIDATE;
      default: return interop::ReadMeta_Role_UNKNOWN_ROLE;
    }
  }

  void run() {
    if (mIsShutdown) {
      SPDLOG_WARN("Sidecar is already down. Will not run again.");
      return;
    }

    std::string server_address(mIpPort);

    ServerBuilder builder;
    /// Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, TlsUtil::buildServerCredentials(mTlsConfOpt));
    /// Register "service" as the instance through which we'll communicate with
    /// clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(this);
    /// Finally assemble the server.
    mServer = builder.BuildAndStart();
    SPDLOG_INFO("Server listening on {}", server_address);
    mServer->Wait();
  }

  /**
   * shut down the server
   */
  void shutdown() {
    if (mIsShutdown) {
      SPDLOG_INFO("Sidecar is already down");
    } else {
      mIsShutdown = true;
      mServer->Shutdown();
    }
  }

 private:
  std::unique_ptr<Server> mServer;
  std::atomic<bool> mIsShutdown = false;

  std::string mIpPort;
  std::optional<TlsConf> mTlsConfOpt;

  /// raft impl
  std::shared_ptr<raft::RaftInterface> mRaftImpl;
};

}  /// namespace app
}  /// namespace gringofts

int main(int argc, char **argv) {
  spdlog::stdout_logger_mt("console");
  spdlog::set_pattern("[%D %H:%M:%S.%F] [%s:%# %!] [%l] [thread %t] %v");
  SPDLOG_INFO("pid={}", getpid());
  assert(argc == 2);
  const auto &configPath = argv[1];
  gringofts::app::Sidecar(configPath).run();
}

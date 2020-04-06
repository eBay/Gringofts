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

#ifndef SRC_APP_UTIL_NETADMINSERVER_H_
#define SRC_APP_UTIL_NETADMINSERVER_H_

#include <INIReader.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>

#include "../infra/util/TlsUtil.h"

#include "generated/grpc/netadmin.grpc.pb.h"
#include "NetAdminServiceProvider.h"

namespace gringofts {
namespace app {

// grpc-related
using ::grpc::Server;
using ::grpc::ServerContext;
using ::grpc::ServerBuilder;
using ::grpc::Status;
using ::grpc::Channel;
using ::grpc::ClientContext;

using gringofts::app::protos::AppNetAdmin;
using gringofts::app::protos::CreateSnapshot_Request;
using gringofts::app::protos::CreateSnapshot_Response;
using gringofts::app::protos::CreateSnapshot_ResponseType;
using gringofts::app::protos::TruncatePrefix_Request;
using gringofts::app::protos::TruncatePrefix_Response;
using gringofts::app::protos::TruncatePrefix_ResponseType;
using gringofts::app::protos::Hotfix_Request;
using gringofts::app::protos::Hotfix_Response;
using gringofts::app::protos::Hotfix_ResponseType;
/**
 * A server class which exposes some management functionalities to external clients, e.g., pubuddy.
 */
class NetAdminServer final : public AppNetAdmin::Service {
 public:
  NetAdminServer(const INIReader &reader,
                 std::shared_ptr<NetAdminServiceProvider> netAdminProxy) :
      mServiceProvider(netAdminProxy),
      mSnapshotTakenCounter(getCounter("snapshot_taken_counter", {})),
      mSnapshotFailedCounter(getCounter("snapshot_failed_counter", {})),
      mPrefixTruncatedCounter(getCounter("prefix_truncated_counter", {})),
      mPrefixTruncateFailedCounter(getCounter("prefix_truncate_failed_counter", {})),
      mHotfixAppliedCounter(getCounter("hotfix_applied_counter", {})),
      mHotfixFailedCounter(getCounter("hotfix_failed_counter", {})),
      mFirstIndexGauge(getGauge("first_index_gauge", {})) {
    mIpPort = reader.Get("netadmin", "ip.port", "UNKNOWN");
    assert(mIpPort != "UNKNOWN");

    mTlsConfOpt = TlsUtil::parseTlsConf(reader, "tls");
  }

  ~NetAdminServer() = default;

  /// disallow copy/move ctor/assignment
  NetAdminServer(const NetAdminServer &) = delete;
  NetAdminServer &operator=(const NetAdminServer &) = delete;

  /**
   * snapshot service, when invoked, will ask app to take a snapshot.
   */
  Status CreateSnapshot(ServerContext *context,
                        const CreateSnapshot_Request *request,
                        CreateSnapshot_Response *reply) override {
    bool expected = false;
    bool ret = mSnapshotIsRunning.compare_exchange_strong(expected, true);
    if (!ret) {
      reply->set_type(CreateSnapshot_ResponseType::CreateSnapshot_ResponseType_PROCESSING);
      return Status::OK;
    }

    SPDLOG_INFO("Start taking a snapshot");
    const auto[succeed, snapshotPath] = mServiceProvider->takeSnapshotAndPersist();
    if (succeed) {
      SPDLOG_INFO("A new snapshot has been persisted to {}", snapshotPath);
      mSnapshotTakenCounter.increase();
    } else {
      SPDLOG_WARN("Failed to create snapshot");
      mSnapshotFailedCounter.increase();
    }
    reply->set_type(succeed ? CreateSnapshot_ResponseType::CreateSnapshot_ResponseType_SUCCESS :
                    CreateSnapshot_ResponseType::CreateSnapshot_ResponseType_FAILED);
    reply->set_message(snapshotPath);

    mSnapshotIsRunning = false;
    return Status::OK;
  }

  /**
   * truncate raft log service, when invoked, will close all the raft segment logs before the specified prefix.
   */
  Status TruncatePrefix(ServerContext *context,
                        const TruncatePrefix_Request *request,
                        TruncatePrefix_Response *reply) override {
    bool expected = false;
    bool ret = mTruncatePrefixIsRunning.compare_exchange_strong(expected, true);
    if (!ret) {
      reply->set_type(TruncatePrefix_ResponseType::TruncatePrefix_ResponseType_PROCESSING);
      return Status::OK;
    }

    /// will set success if everything is OK
    reply->set_type(TruncatePrefix_ResponseType::TruncatePrefix_ResponseType_FAILED);
    std::string errorMessage = "Unknown reason";

    do {
      /// 1, get offset of latest snapshot
      auto offsetOpt = mServiceProvider->getLatestSnapshotOffset();
      if (!offsetOpt) {
        errorMessage = "Refuse truncate prefix, since no snapshot is founded.";
        SPDLOG_WARN("{}", errorMessage);
        mPrefixTruncateFailedCounter.increase();
        break;
      }

      auto snapshotOffset = *offsetOpt;

      /// 2, get firstIndexKept
      auto firstIndexKept = request->firstindexkept();

      /// 3, compare
      if (firstIndexKept > snapshotOffset) {
        errorMessage = "Refuse truncate prefix, since firstIndexKept > snapshotOffset";
        SPDLOG_WARN("{}, Detail: {} > {}.", errorMessage, firstIndexKept, snapshotOffset);
        mPrefixTruncateFailedCounter.increase();
        break;
      }

      /// 4, call truncatePrefix
      mServiceProvider->truncatePrefix(firstIndexKept);

      reply->set_type(TruncatePrefix_ResponseType::TruncatePrefix_ResponseType_SUCCESS);
      errorMessage = "OK";

      SPDLOG_WARN("Truncate prefix to {}, meanwhile, latest snapshot offset is {}",
                  firstIndexKept, snapshotOffset);
      mPrefixTruncatedCounter.increase();
      mFirstIndexGauge.set(firstIndexKept);
    } while (0);

    reply->set_message(errorMessage);

    mTruncatePrefixIsRunning = false;
    return Status::OK;
  }

  /**
   * hotfix service, when invoked, will ask app to execute some hotfix.
   */
  Status Hotfix(ServerContext *context,
                const Hotfix_Request *request,
                Hotfix_Response *reply) override {
    bool expected = false;
    bool ret = mHotfixIsRunning.compare_exchange_strong(expected, true);
    if (!ret) {
      reply->set_type(Hotfix_ResponseType::Hotfix_ResponseType_PROCESSING);
      return Status::OK;
    }

    SPDLOG_INFO("Start doing hotfix");
    auto succeed = mServiceProvider->executeHotfix(request->payload());
    auto message = "Hotfix has been done successfully";
    if (!succeed) {
      message = "Failed to apply the hotfix";
      mHotfixFailedCounter.increase();
    } else {
      mHotfixAppliedCounter.increase();
    }
    SPDLOG_INFO(message);
    reply->set_type(succeed ? Hotfix_ResponseType::Hotfix_ResponseType_SUCCESS :
                    Hotfix_ResponseType::Hotfix_ResponseType_FAILED);
    reply->set_message(message);

    mHotfixIsRunning = false;
    return Status::OK;
  }

  /**
   * The main function of the dedicated thread
   */
  void run() {
    if (mIsShutdown) {
      SPDLOG_WARN("NetAdmin server is already down. Will not run again.");
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
  }

  /**
   * shut down the server
   */
  void shutdown() {
    if (mIsShutdown) {
      SPDLOG_INFO("NetAdmin server is already down");
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

  std::shared_ptr<NetAdminServiceProvider> mServiceProvider;
  std::atomic<bool> mSnapshotIsRunning = false;
  std::atomic<bool> mTruncatePrefixIsRunning = false;
  std::atomic<bool> mHotfixIsRunning = false;

  /// metrics start
  mutable santiago::MetricsCenter::CounterType mSnapshotTakenCounter;
  mutable santiago::MetricsCenter::CounterType mSnapshotFailedCounter;
  mutable santiago::MetricsCenter::CounterType mPrefixTruncatedCounter;
  mutable santiago::MetricsCenter::CounterType mPrefixTruncateFailedCounter;
  mutable santiago::MetricsCenter::CounterType mHotfixAppliedCounter;
  mutable santiago::MetricsCenter::CounterType mHotfixFailedCounter;
  mutable santiago::MetricsCenter::GaugeType mFirstIndexGauge;
  /// metrics end
};

}  /// namespace app
}  /// namespace gringofts

#endif  // SRC_APP_UTIL_NETADMINSERVER_H_

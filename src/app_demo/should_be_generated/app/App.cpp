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

#include "App.h"

#include "../../v1/AppStateMachine.h"

#include "../../../infra/es/store/DefaultCommandEventStore.h"
#include "../../../infra/es/store/RaftCommandEventStore.h"
#include "../../../infra/es/store/ReadonlyDefaultCommandEventStore.h"
#include "../../../infra/es/store/ReadonlyRaftCommandEventStore.h"
#include "../../../infra/es/store/ReadonlySQLiteCommandEventStore.h"
#include "../../../infra/es/store/SQLiteCommandEventStore.h"
#include "../../../infra/monitor/MonitorCenter.h"
#include "../../../infra/raft/RaftBuilder.h"
#include "../../../infra/raft/metrics/RaftMonitorAdaptor.h"

namespace gringofts {
namespace demo {

App::App(const char *configPath) : mIsShutdown(false) {
  SPDLOG_INFO("current working dir: {}", FileUtil::currentWorkingDir());

  INIReader reader(configPath);
  if (reader.ParseError() < 0) {
    SPDLOG_WARN("Cannot load config file {}, exiting", configPath);
    throw std::runtime_error("Cannot load config file");
  }

  app::AppInfo::init(reader);

  mCrypto = std::make_shared<gringofts::CryptoUtil>();
  mCrypto->init(reader);

  initDeploymentMode(reader);

  initMonitor(reader);

  initCommandEventStore(reader);

  std::string snapshotDir = reader.Get("snapshot", "dir", "UNKNOWN");
  assert(snapshotDir != "UNKNOWN");

  auto commandEventDecoder = std::make_shared<app::CommandEventDecoderImpl<EventDecoderImpl, CommandDecoderImpl>>();

  const auto &appVersion = app::AppInfo::appVersion();
  if (appVersion == "v1") {
    mEventApplyLoop = std::make_shared<app::EventApplyLoop<v1::AppStateMachine>>(
        reader, commandEventDecoder, std::move(mReadonlyCommandEventStoreForEventApplyLoop), snapshotDir);
    mCommandProcessLoop = std::make_unique<CommandProcessLoop<v1::AppStateMachine>>(
        reader,
        commandEventDecoder,
        mDeploymentMode,
        mEventApplyLoop,
        mCommandQueue,
        std::move(mReadonlyCommandEventStoreForCommandProcessLoop),
        mCommandEventStore,
        snapshotDir);
  } else if (appVersion == "v2") {
    mEventApplyLoop =
        std::make_shared<app::EventApplyLoop<v2::RocksDBBackedAppStateMachine>>(
            reader, commandEventDecoder, std::move(mReadonlyCommandEventStoreForEventApplyLoop), snapshotDir);
    mCommandProcessLoop = std::make_unique<CommandProcessLoop<v2::MemoryBackedAppStateMachine>>(
        reader,
        commandEventDecoder,
        mDeploymentMode,
        mEventApplyLoop,
        mCommandQueue,
        std::move(mReadonlyCommandEventStoreForCommandProcessLoop),
        mCommandEventStore,
        snapshotDir);
  } else {
    SPDLOG_ERROR("App version {} is not supported. Exiting...", appVersion);
    assert(0);
  }

  mRequestReceiver = ::std::make_unique<RequestReceiver>(reader, mCommandQueue);
  mNetAdminServer = ::std::make_unique<app::NetAdminServer>(reader, mEventApplyLoop);
  mPostServer = std::make_unique<BundleExposePublisher>(reader, std::move(mReadonlyCommandEventStoreForPostServer));
}

App::~App() {
  SPDLOG_INFO("deleting app");
}

void App::initDeploymentMode(const INIReader &reader) {
  std::string mode = reader.Get("app", "deployment.mode", "standalone");
  if (mode == "standalone") {
    mDeploymentMode = DeploymentMode::Standalone;
  } else if (mode == "distributed") {
    mDeploymentMode = DeploymentMode::Distributed;
  } else {
    SPDLOG_ERROR("Unrecognized deployment mode: {}. Exiting...", mode);
    throw std::runtime_error("Unrecognized deployment mode");
  }

  SPDLOG_INFO("Deployment mode is {}", mode);
}

void App::initMonitor(const INIReader &reader) {
  int monitorPort = reader.GetInteger("monitor", "port", -1);
  assert(monitorPort > 0);
  auto &server = gringofts::getMonitorServer("0.0.0.0", monitorPort);

  auto &appInfo = Singleton<santiago::AppInfo>::getInstance();
  auto appName = "demoApp";
  auto appVersion = Util::getCurrentVersion();
  auto appEnv = reader.Get("app", "env", "unknown");
  appInfo.setAppInfo(appName, appVersion, appEnv);

  auto startTime = TimeUtil::currentTimeInNanos();
  appInfo.gauge("start_time_guage", {}).set(startTime);

  server.Registry(appInfo);
  SPDLOG_INFO("Init monitor with app name : {} , app version : {}, app env : {}, start time : {}",
              appName,
              appVersion,
              appEnv,
              startTime);
}

void App::initCommandEventStore(const INIReader &reader) {
  std::string storeType = reader.Get("store", "persistence.type", "UNKNOWN");
  assert(storeType != "UNKNOWN");
  if (storeType == "default") {
    assert(mDeploymentMode == DeploymentMode::Standalone);

    mCommandEventStore = std::make_shared<DefaultCommandEventStore>();
    mReadonlyCommandEventStoreForCommandProcessLoop = std::make_unique<ReadonlyDefaultCommandEventStore>();
    mReadonlyCommandEventStoreForEventApplyLoop = std::make_unique<ReadonlyDefaultCommandEventStore>();
    mReadonlyCommandEventStoreForPostServer = std::make_unique<ReadonlyDefaultCommandEventStore>();
  } else if (storeType == "sqlite") {
    assert(mDeploymentMode == DeploymentMode::Standalone);

    std::string configPath = reader.Get("store", "sqlite.path", "UNKNOWN");
    assert(configPath != "UNKNOWN");
    auto sqliteDao = std::make_shared<SQLiteStoreDao>(configPath.c_str());
    std::shared_ptr<IdGenerator> commandIdGenerator = std::make_shared<IdGenerator>();
    std::shared_ptr<IdGenerator> eventIdGenerator = std::make_shared<IdGenerator>();
    mCommandEventStore =
        std::make_shared<SQLiteCommandEventStore>(sqliteDao, commandIdGenerator, eventIdGenerator);
    mReadonlyCommandEventStoreForCommandProcessLoop =
        std::make_unique<ReadonlySQLiteCommandEventStore>(sqliteDao,
                                                          true,
                                                          commandIdGenerator,
                                                          eventIdGenerator);
    mReadonlyCommandEventStoreForEventApplyLoop = std::make_unique<ReadonlySQLiteCommandEventStore>(sqliteDao,
                                                                                                    false,
                                                                                                    nullptr,
                                                                                                    nullptr);
    mReadonlyCommandEventStoreForPostServer = std::make_unique<ReadonlySQLiteCommandEventStore>(sqliteDao,
                                                                                                false,
                                                                                                nullptr,
                                                                                                nullptr);
  } else if (storeType == "raft") {
    assert(mDeploymentMode == DeploymentMode::Distributed);

    std::string configPath = reader.Get("store", "raft.config.path", "UNKNOWN");
    assert(configPath != "UNKNOWN");

    std::shared_ptr<app::CommandEventDecoderImpl<EventDecoderImpl, CommandDecoderImpl>> commandEventDecoder =
        std::make_shared<app::CommandEventDecoderImpl<EventDecoderImpl, CommandDecoderImpl>>();
    auto raftImpl = raft::buildRaftImpl(configPath.c_str(), std::nullopt);
    auto metricsAdaptor = std::make_shared<RaftMonitorAdaptor>(raftImpl);
    enableMonitorable(metricsAdaptor);
    mCommandEventStore = std::make_shared<RaftCommandEventStore>(raftImpl, mCrypto);
    mReadonlyCommandEventStoreForCommandProcessLoop = nullptr;
    mReadonlyCommandEventStoreForEventApplyLoop = std::make_unique<ReadonlyRaftCommandEventStore>(raftImpl,
                                                                                                  commandEventDecoder,
                                                                                                  commandEventDecoder,
                                                                                                  mCrypto,
                                                                                                  true);
    mReadonlyCommandEventStoreForPostServer = std::make_unique<ReadonlyRaftCommandEventStore>(raftImpl,
                                                                                              commandEventDecoder,
                                                                                              commandEventDecoder,
                                                                                              mCrypto,
                                                                                              false);
  }
}

void App::startRequestReceiver() {
  mServerThread = std::thread([this]() {
    pthread_setname_np(pthread_self(), "ReqReceiver");
    mRequestReceiver->run();
  });
}

void App::startNetAdminServer() {
  mNetAdminServerThread = std::thread([this]() {
    pthread_setname_np(pthread_self(), "NetAdmin");
    mNetAdminServer->run();
  });
}

void App::startProcessCommandLoop() {
  mCommandProcessLoopThread = std::thread([this]() {
    pthread_setname_np(pthread_self(), "CommandProcLoop");
    switch (mDeploymentMode) {
      case DeploymentMode::Standalone:mCommandProcessLoop->run();
        break;
      case DeploymentMode::Distributed:mCommandProcessLoop->runDistributed();
        break;
    }
  });
}

void App::startEventApplyLoop() {
  mEventApplyLoopThread = std::thread([this]() {
    pthread_setname_np(pthread_self(), "EventApplyLoop");
    mEventApplyLoop->run();
  });
}

void App::startPersistLoop() {
  mPersistLoopThread = std::thread([this]() {
    pthread_setname_np(pthread_self(), "CmdEvtStoreMain");
    mCommandEventStore->run();
  });
}

void App::startPostServerLoop() {
  mPostServerThread = std::thread([this]() {
    pthread_setname_np(pthread_self(), "PostServerMain");
    if (mPostServer != nullptr) {
      mPostServer->run();
    }
  });
}

void App::run() {
  if (mIsShutdown) {
    SPDLOG_WARN("App is already down. Will not run again.");
  } else {
    if (mDeploymentMode == DeploymentMode::Standalone) {
      mCommandProcessLoop->recoverOnce();
    }

    // now run the threads
    startRequestReceiver();
    startNetAdminServer();
    startPersistLoop();
    startEventApplyLoop();
    startProcessCommandLoop();
    startPostServerLoop();

    // wait for all threads to exit
    mPostServerThread.join();
    mCommandProcessLoopThread.join();
    mEventApplyLoopThread.join();
    mPersistLoopThread.join();
    mNetAdminServerThread.join();
    mServerThread.join();
  }
}

void App::shutdown() {
  if (mIsShutdown) {
    SPDLOG_INFO("App is already down");
  } else {
    mIsShutdown = true;

    // shutdown all threads
    mCommandQueue.shutdown();
    mRequestReceiver->shutdown();
    mNetAdminServer->shutdown();
    mCommandProcessLoop->shutdown();
    mEventApplyLoop->shutdown();
    mCommandEventStore->shutdown();
    if (mPostServer != nullptr) {
      mPostServer->shutdown();
    }
  }
}

}  /// namespace demo
}  /// namespace gringofts

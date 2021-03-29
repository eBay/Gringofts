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

#include "KVEngineImpl.h"

#include <future>

#include <app_util/AppInfo.h>
#include <infra/monitor/MonitorCenter.h>
#include <infra/raft/RaftBuilder.h>
#include <infra/raft/metrics/RaftMonitorAdaptor.h>

#include "execution/ExecutionServiceImpl.h"
#include "model/ConnectCommand.h"
#include "model/KVDeleteCommand.h"
#include "model/KVGetCommand.h"
#include "model/KVPutCommand.h"
#include "model/KVTransCommand.h"
#include "model/KVImportCommand.h"
#include "store/WSConcurInMemoryKVStore.h"
#include "store/InMemoryKVStore.h"
#include "utils/AppUtil.h"

namespace goblin::kvengine {

utils::Status KVEngineImpl::Init(
     const char *configPath,
     const std::vector<store::WSName> &initWSNames,
     store::WSLookupFunc wsLookupFunc,
     execution::BecomeLeaderCallBack becomeLeaderCallBack,
     execution::PreExecuteCallBack preExecuteCallBack,
     const std::shared_ptr<store::KVObserver>& observerSharedPtr) {
  INIReader reader(configPath);
  if (reader.ParseError() < 0) {
    SPDLOG_WARN("Cannot load config file {}, exiting", configPath);
    throw std::runtime_error("Cannot load config file");
  }
  gringofts::app::AppInfo::init(reader);

  std::string raftConfigPath = reader.Get("store", "raft.config.path", "UNKNOWN");
  assert(raftConfigPath != "UNKNOWN");
  mRaftImpl = gringofts::raft::buildRaftImpl(
      raftConfigPath.c_str(), std::nullopt);
  auto metricsAdaptor = std::make_shared<gringofts::RaftMonitorAdaptor>(mRaftImpl);
  gringofts::enableMonitorable(metricsAdaptor);

  mCrypto = std::make_shared<gringofts::CryptoUtil>();
  mCrypto->init(reader);

  initMonitor(reader);

  const auto &walDir = reader.Get("rocksdb", "wal.dir", "");
  const auto &dbDir = reader.Get("rocksdb", "db.dir", "");
  mVersionStore = std::make_shared<store::VersionStore>();
  mRocksDBKVStore = std::make_shared<store::RocksDBKVStore>(walDir, dbDir, initWSNames, wsLookupFunc);
  if (observerSharedPtr != nullptr) {
    mRocksDBKVStore->registerObserver(observerSharedPtr);
  }
  mReplyLoop = std::make_shared<raft::ReplyLoop>(mVersionStore, mRaftImpl);
  mRaftEventStore = std::make_shared<raft::RaftEventStore>(mRaftImpl, mCrypto, mReplyLoop);
  mEventApplyLoop = std::make_shared<execution::EventApplyLoop>(mRaftEventStore, mRocksDBKVStore);

  startProcessCommandLoop(reader, initWSNames, wsLookupFunc, becomeLeaderCallBack, preExecuteCallBack);

  startEventApplyLoop();
  return utils::Status::ok();
}

utils::Status KVEngineImpl::Destroy() {
  SPDLOG_INFO("Shutting down App");
  mCommandProcessExecutionService->shutdown();
  mEventApplyLoop->shutdown();
  mEventApplyLoopThread.join();
  return utils::Status::ok();
}

KVEngineImpl::~KVEngineImpl() {
  SPDLOG_INFO("deleting kv engine impl");
}

void KVEngineImpl::startProcessCommandLoop(
    const INIReader &reader,
    const std::vector<store::WSName> &initWSNames,
    store::WSLookupFunc wsLookupFunc,
    execution::BecomeLeaderCallBack becomeLeaderCallBack,
    execution::PreExecuteCallBack preExecuteCallBack) {
  const uint32_t concurLevel = 16;
  auto readOnlyStore = std::make_shared<store::ReadOnlyKVStore>(mRocksDBKVStore);
  std::shared_ptr<store::ProxyKVStore> memoryStore = nullptr;
  if (concurLevel > 1) {
    memoryStore = std::make_shared<store::WSConcurInMemoryKVStore>(readOnlyStore, initWSNames, wsLookupFunc);
  } else {
    memoryStore = std::make_shared<store::InMemoryKVStore>(readOnlyStore);
  }
  auto commandProcessor = std::make_shared<execution::CommandProcessor>(
      mVersionStore,
      memoryStore,
      mRaftEventStore,
      becomeLeaderCallBack,
      preExecuteCallBack);
  mCommandProcessExecutionService = std::make_shared<execution::ExecutionServiceImpl<CommandPtr>>(
      commandProcessor, concurLevel);
  // cache eviction for in memory kv store
  int64_t capacity = reader.GetInteger("app", "cache.capacity", strategy::LRUEviction::kDefaultCapacity);
  int64_t optimal = reader.GetInteger("app", "cache.optimal", strategy::LRUEviction::kDefaultOptimal);
  assert(capacity > 0 && optimal > 0);

  auto cacheProxy = std::make_shared<strategy::CacheEvictionProxy<strategy::LRUEviction>>
      (*mEventApplyLoop, *mCommandProcessExecutionService, capacity, optimal);
  cacheProxy->start();
  mCacheEviction = cacheProxy;
  memoryStore->registerObserver(mCacheEviction);

  mCommandProcessExecutionService->start();
}

void KVEngineImpl::startEventApplyLoop() {
  mEventApplyLoopThread = std::thread([this]() {
    pthread_setname_np(pthread_self(), "EventApplyLoop");
    mEventApplyLoop->run();
    SPDLOG_INFO("event apply loop finishes");
  });
}

void KVEngineImpl::initMonitor(const INIReader &reader) {
  int monitorPort = reader.GetInteger("monitor", "port", -1);
  if (monitorPort < 0) {
    SPDLOG_WARN("monitor is not enabled");
    return;
  }
  assert(monitorPort > 0);
  auto &server = gringofts::getMonitorServer("0.0.0.0", monitorPort);

  auto &appInfo = gringofts::Singleton<santiago::AppInfo>::getInstance();
  auto appName = reader.Get("app", "name", "GoblinApp");
  auto appVersion = utils::AppUtil::getCurrentVersion();
  auto appEnv = reader.Get("app", "env", "unknown");
  appInfo.setAppInfo(appName, appVersion, appEnv);

  auto startTime = gringofts::TimeUtil::currentTimeInNanos();
  appInfo.gauge("start_time_guage", {}).set(startTime);

  server.Registry(appInfo);
  // register monitor center
  server.Registry(gringofts::Singleton<gringofts::MonitorCenter>::getInstance());
  SPDLOG_INFO("Init monitor with app name : {} , app version : {}, app env : {}, start time : {}",
              appName,
              appVersion,
              appEnv,
              startTime);
}

utils::Status KVEngineImpl::InitForTest(
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
     gringofts::TestPointProcessor *processor) {
  INIReader reader(configPath);
  if (reader.ParseError() < 0) {
    SPDLOG_WARN("Cannot load config file {}, exiting", configPath);
    throw std::runtime_error("Cannot load config file");
  }
  SPDLOG_INFO("init for kvengineimpl in test");

  mRaftImpl = raftInst;

  mVersionStore = versionStorePtr;
  mRocksDBKVStore = rocksdbPtr;
  mReplyLoop = replyLoopPtr;
  mRaftEventStore = eventStorePtr;
  mEventApplyLoop = eventApplyPtr;

  startProcessCommandLoop(reader, initWSNames, wsLookupFunc, becomeLeaderCallBack, preExecuteCallBack);
  startEventApplyLoop();

  mTPProcessor = processor;

  return utils::Status::ok();
}

proto::Connect::Response KVEngineImpl::connect(const proto::Connect::Request &req) {
  auto promise = std::make_shared<std::promise<proto::Connect::Response>>();
  auto future = promise->get_future();
  connectAsync(req, [promise](const proto::Connect::Response &resp) {
      promise->set_value(resp);
      });
  return future.get();
}

proto::Put::Response KVEngineImpl::put(const proto::Put::Request &req) {
  auto promise = std::make_shared<std::promise<proto::Put::Response>>();
  auto future = promise->get_future();
  putAsync(req, [promise](const proto::Put::Response &resp) {
      promise->set_value(resp);
      });
  return future.get();
}

proto::Get::Response KVEngineImpl::get(const proto::Get::Request &req) {
  auto promise = std::make_shared<std::promise<proto::Get::Response>>();
  auto future = promise->get_future();
  getAsync(req, [promise](const proto::Get::Response &resp) {
      promise->set_value(resp);
      });
  return future.get();
}

proto::Delete::Response KVEngineImpl::remove(const proto::Delete::Request &req) {
  auto promise = std::make_shared<std::promise<proto::Delete::Response>>();
  auto future = promise->get_future();
  removeAsync(req, [promise](const proto::Delete::Response &resp) {
      promise->set_value(resp);
      });
  return future.get();
}

proto::ExeBatch::Response KVEngineImpl::exeBatch(const proto::ExeBatch::Request &req) {
  auto promise = std::make_shared<std::promise<proto::ExeBatch::Response>>();
  auto future = promise->get_future();
  exeBatchAsync(req, [promise](const proto::ExeBatch::Response &resp) {
      promise->set_value(resp);
      });
  return future.get();
}

/// TODO: support read from followers with specific version
proto::ExeBatch::Response KVEngineImpl::getAppliedBatch(const proto::ExeBatch::Request &req) {
  proto::ExeBatch::Response resp;
  resp.mutable_header()->set_code(proto::ResponseCode::OK);
  for (auto entry : req.entries()) {
    if (entry.has_readentry()) {
      auto readEntry = entry.readentry();
      store::ValueType value;
      store::TTLType ttl;
      store::VersionType version;
      auto s = mRocksDBKVStore->readKV(readEntry.key(), &value, &ttl, &version);
      auto readResult = resp.add_results()->mutable_readresult();
      if (s.isOK()) {
        readResult->set_code(proto::ResponseCode::OK);
      } else if (s.isNotFound()) {
        readResult->set_code(proto::ResponseCode::KEY_NOT_EXIST);
      } else {
        readResult->set_code(proto::ResponseCode::GENERAL_ERROR);
      }
      readResult->set_message(s.getDetail());
      readResult->set_version(version);
      readResult->set_value(value);
    } else {
      resp.mutable_header()->set_code(proto::ResponseCode::BAD_REQUEST);
    }
  }
  return resp;
}

proto::Transaction::Response KVEngineImpl::trans(const proto::Transaction::Request &req) {
  auto promise = std::make_shared<std::promise<proto::Transaction::Response>>();
  auto future = promise->get_future();
  transAsync(req, [promise](const proto::Transaction::Response &resp) {
      promise->set_value(resp);
      });
  return future.get();
}

utils::Status KVEngineImpl::connectAsync(
    const proto::Connect::Request& req,
    std::function<void(const proto::Connect::Response&)> cb) {
  auto context = std::make_shared<model::ConnectCommandContext>(req, cb);
  auto command = std::make_shared<model::ConnectCommand>(context);
  try {
    // set initial timestamp
    context->setCreateTimeInNanos();
    mCommandProcessExecutionService->submit(command);
    return utils::Status::ok();
  } catch (const gringofts::QueueStoppedException &e) {
    SPDLOG_WARN(e.what());
    context->fillResponseAndReply(proto::ResponseCode::GENERAL_ERROR, std::string(e.what()), std::nullopt);
    return utils::Status::stopped();
  }
}

utils::Status KVEngineImpl::putAsync(
    const proto::Put::Request &req,
    std::function<void(const proto::Put::Response&)> cb) {
  auto context = std::make_shared<model::KVPutCommandContext>(req, cb);
  auto command = std::make_shared<model::KVPutCommand>(context);
  try {
    // set initial timestamp
    context->setCreateTimeInNanos();
    mCommandProcessExecutionService->submit(command);
    return utils::Status::ok();
  } catch (const gringofts::QueueStoppedException &e) {
    SPDLOG_WARN(e.what());
    context->fillResponseAndReply(proto::ResponseCode::GENERAL_ERROR, std::string(e.what()), std::nullopt);
    return utils::Status::stopped();
  }
}

utils::Status KVEngineImpl::importAsync(
    const proto::Put::Request &req,
    std::function<void(const proto::Put::Response&)> cb) {
  auto context = std::make_shared<model::KVPutCommandContext>(req, cb);
  auto command = std::make_shared<model::KVImportCommand>(context);
  try {
    // set initial timestamp
    context->setCreateTimeInNanos();
    mCommandProcessExecutionService->submit(command);
    return utils::Status::ok();
  } catch (const gringofts::QueueStoppedException &e) {
    SPDLOG_WARN(e.what());
    context->fillResponseAndReply(proto::ResponseCode::GENERAL_ERROR, std::string(e.what()), std::nullopt);
    return utils::Status::stopped();
  }
}

utils::Status KVEngineImpl::getAsync(
    const proto::Get::Request &req,
    std::function<void(const proto::Get::Response&)> cb) {
  auto context = std::make_shared<model::KVGetCommandContext>(req, cb);
  auto command = std::make_shared<model::KVGetCommand>(context);
  try {
    // set initial timestamp
    context->setCreateTimeInNanos();
    mCommandProcessExecutionService->submit(command);
    return utils::Status::ok();
  } catch (const gringofts::QueueStoppedException &e) {
    SPDLOG_WARN(e.what());
    context->fillResponseAndReply(proto::ResponseCode::GENERAL_ERROR, std::string(e.what()), std::nullopt);
    return utils::Status::stopped();
  }
}

utils::Status KVEngineImpl::removeAsync(
    const proto::Delete::Request &req,
    std::function<void(const proto::Delete::Response&)> cb) {
  auto context = std::make_shared<model::KVDeleteCommandContext>(req, cb);
  auto command = std::make_shared<model::KVDeleteCommand>(context);
  try {
    // set initial timestamp
    context->setCreateTimeInNanos();
    mCommandProcessExecutionService->submit(command);
    return utils::Status::ok();
  } catch (const gringofts::QueueStoppedException &e) {
    SPDLOG_WARN(e.what());
    context->fillResponseAndReply(proto::ResponseCode::GENERAL_ERROR, std::string(e.what()), std::nullopt);
    return utils::Status::stopped();
  }
}

utils::Status KVEngineImpl::exeBatchAsync(
    const proto::ExeBatch::Request &req,
    std::function<void(const proto::ExeBatch::Response&)> cb) {
  std::map<uint32_t, std::future<proto::Put::Response>> putFutures;
  std::map<uint32_t, std::future<proto::Get::Response>> getFutures;
  std::map<uint32_t, std::future<proto::Delete::Response>> deleteFutures;
  uint32_t i = 0;
  auto entryNum = req.entries().size();
  for (auto entry : req.entries()) {
    if (entry.has_writeentry()) {
      auto promise = std::make_shared<std::promise<proto::Put::Response>>();
      auto future = promise->get_future();
      proto::Put::Request putReq;
      *putReq.mutable_header() = req.header();
      *putReq.mutable_entry() = entry.writeentry();
      putAsync(putReq, [promise](const proto::Put::Response &resp) {
          promise->set_value(resp);
          });
      putFutures[i++] = std::move(future);
    } else if (entry.has_readentry()) {
      auto promise = std::make_shared<std::promise<proto::Get::Response>>();
      auto future = promise->get_future();
      proto::Get::Request getReq;
      *getReq.mutable_header() = req.header();
      *getReq.mutable_entry() = entry.readentry();
      getAsync(getReq, [promise](const proto::Get::Response &resp) {
          promise->set_value(resp);
          });
      getFutures[i++] = std::move(future);
    } else if (entry.has_removeentry()) {
      auto promise = std::make_shared<std::promise<proto::Delete::Response>>();
      auto future = promise->get_future();
      proto::Delete::Request deleteReq;
      *deleteReq.mutable_header() = req.header();
      *deleteReq.mutable_entry() = entry.removeentry();
      removeAsync(deleteReq, [promise](const proto::Delete::Response &resp) {
          promise->set_value(resp);
          });
      deleteFutures[i++] = std::move(future);
    } else {
      return utils::Status::invalidArg();
    }
  }
  proto::ExeBatch::Response resp;
  resp.mutable_header()->set_code(proto::ResponseCode::OK);
  for (auto i = 0; i < entryNum; ++i) {
    auto putIt = putFutures.find(i);
    if (putIt != putFutures.end()) {
      auto putResp = putIt->second.get();
      *resp.add_results()->mutable_writeresult() = putResp.result();
      if (putResp.header().code() != proto::ResponseCode::OK) {
        *resp.mutable_header() = putResp.header();
      }
      continue;
    }
    auto getIt = getFutures.find(i);
    if (getIt != getFutures.end()) {
      auto getResp = getIt->second.get();
      *resp.add_results()->mutable_readresult() = getResp.result();
      if (getResp.header().code() != proto::ResponseCode::OK) {
        *resp.mutable_header() = getResp.header();
      }
      continue;
    }
    auto deleteIt = deleteFutures.find(i);
    if (deleteIt != deleteFutures.end()) {
      auto deleteResp = deleteIt->second.get();
      *resp.add_results()->mutable_removeresult() = deleteResp.result();
      if (deleteResp.header().code() != proto::ResponseCode::OK) {
        *resp.mutable_header() = deleteResp.header();
      }
      continue;
    }
    /// should not come here
    assert(0);
  }
  cb(resp);
  return utils::Status::ok();
}

utils::Status KVEngineImpl::transAsync(
    const proto::Transaction::Request &req,
    std::function<void(const proto::Transaction::Response&)> cb) {
  auto context = std::make_shared<model::KVTransCommandContext>(req, cb);
  auto command = std::make_shared<model::KVTransCommand>(context);
  try {
    // set initial timestamp
    context->setCreateTimeInNanos();
    mCommandProcessExecutionService->submit(command);
    return utils::Status::ok();
  } catch (const gringofts::QueueStoppedException &e) {
    SPDLOG_WARN(e.what());
    context->fillResponseAndReply(proto::ResponseCode::GENERAL_ERROR, std::string(e.what()), std::nullopt);
    return utils::Status::stopped();
  }
}

utils::Status KVEngineImpl::migrateBatchAsync(
    const proto::MigrateBatch::Request &req,
    std::function<void(const proto::MigrateBatch::Response&)> cb) {
  std::vector<std::future<proto::Put::Response>> putFutures;
  auto entryNum = req.entries().size();
  for (auto entry : req.entries()) {
    auto promise = std::make_shared<std::promise<proto::Put::Response>>();
    auto future = promise->get_future();
    proto::Put::Request putReq;
    *putReq.mutable_entry() = entry;
    importAsync(putReq, [promise](const proto::Put::Response &resp) {
        promise->set_value(resp);
        });
    putFutures.push_back(std::move(future));
  }
  proto::MigrateBatch::Response resp;
  for (auto i = 0; i < entryNum; ++i) {
    auto putResp = putFutures[i].get();
    *resp.add_results() = putResp.result();
  }
  cb(resp);
  return utils::Status::ok();
}

utils::Status KVEngineImpl::exeCustomCommand(std::shared_ptr<model::Command> cmd) {
  auto context = cmd->getContext();
  try {
    // set initial timestamp
    context->setCreateTimeInNanos();
    mCommandProcessExecutionService->submit(cmd);
    return utils::Status::ok();
  } catch (const gringofts::QueueStoppedException &e) {
    SPDLOG_WARN(e.what());
    context->fillResponseAndReply(proto::ResponseCode::GENERAL_ERROR, std::string(e.what()), std::nullopt);
    return utils::Status::stopped();
  }
}
}  /// namespace goblin::kvengine

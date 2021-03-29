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

#include "KVEngine.h"

#include "KVEngineImpl.h"

namespace goblin::kvengine {

utils::Status KVEngine::Init(
     const char *configPath,
     const std::vector<store::WSName> &initWSNames,
     store::WSLookupFunc wsLookupFunc,
     execution::BecomeLeaderCallBack becomeLeaderCallBack,
     execution::PreExecuteCallBack preExecuteCallBack,
     const std::shared_ptr<store::KVObserver> &observerSharedPtr) {
  mImpl = std::make_shared<KVEngineImpl>();
  return mImpl->Init(configPath, initWSNames, wsLookupFunc, becomeLeaderCallBack,
      preExecuteCallBack, observerSharedPtr);
}

utils::Status KVEngine::Destroy() {
  return mImpl->Destroy();
}

proto::Connect::Response KVEngine::connect(const proto::Connect::Request &req) {
  return mImpl->connect(req);
}

proto::Put::Response KVEngine::put(const proto::Put::Request &req) {
  return mImpl->put(req);
}

proto::Get::Response KVEngine::get(const proto::Get::Request &req) {
  return mImpl->get(req);
}

proto::Delete::Response KVEngine::remove(const proto::Delete::Request &req) {
  return mImpl->remove(req);
}

proto::ExeBatch::Response KVEngine::exeBatch(const proto::ExeBatch::Request &req) {
  return mImpl->exeBatch(req);
}

proto::Transaction::Response KVEngine::trans(const proto::Transaction::Request &req) {
  return mImpl->trans(req);
}

proto::ExeBatch::Response KVEngine::getAppliedBatch(const proto::ExeBatch::Request &req) {
  return mImpl->getAppliedBatch(req);
}

utils::Status KVEngine::connectAsync(
    const proto::Connect::Request& req,
    std::function<void(const proto::Connect::Response&)> cb) {
  return mImpl->connectAsync(req, cb);
}

utils::Status KVEngine::putAsync(
    const proto::Put::Request& req,
    std::function<void(const proto::Put::Response&)> cb) {
  return mImpl->putAsync(req, cb);
}

utils::Status KVEngine::getAsync(
    const proto::Get::Request &req,
    std::function<void(const proto::Get::Response&)> cb) {
  return mImpl->getAsync(req, cb);
}

utils::Status KVEngine::removeAsync(
    const proto::Delete::Request &req,
    std::function<void(const proto::Delete::Response&)> cb) {
  return mImpl->removeAsync(req, cb);
}

utils::Status KVEngine::exeBatchAsync(
    const proto::ExeBatch::Request &req,
    std::function<void(const proto::ExeBatch::Response&)> cb) {
  return mImpl->exeBatchAsync(req, cb);
}

utils::Status KVEngine::transAsync(
    const proto::Transaction::Request &req,
    std::function<void(const proto::Transaction::Response&)> cb) {
  return mImpl->transAsync(req, cb);
}

utils::Status KVEngine::migrateBatchAsync(
    const proto::MigrateBatch::Request &req,
    std::function<void(const proto::MigrateBatch::Response&)> cb) {
  return mImpl->migrateBatchAsync(req, cb);
}

utils::Status KVEngine::exeCustomCommand(std::shared_ptr<model::Command> cmd) {
  return mImpl->exeCustomCommand(cmd);
}

utils::Status KVEngine::InitForTest(std::shared_ptr<KVEngineImpl> impl) {
  mImpl = impl;
  return utils::Status::ok();
}

}  /// namespace goblin::kvengine

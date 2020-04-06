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

#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_APP_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_APP_H_

#include <tuple>

#include <INIReader.h>
#include <spdlog/spdlog.h>

#include "../../../app_util/AppInfo.h"
#include "../../../app_util/CommandEventDecoderImpl.h"
#include "../../../app_util/EventApplyLoop.h"
#include "../../../app_util/NetAdminServer.h"
#include "../../../infra/es/Command.h"
#include "../../../infra/es/CommandEventStore.h"
#include "../../../infra/util/CryptoUtil.h"

#include "../../AppStateMachine.h"
#include "../domain/CommandDecoderImpl.h"
#include "../domain/CommandProcessLoop.h"
#include "../domain/EventDecoderImpl.h"
#include "../domain/common_types.h"
#include "../domain/post/BundleExposePublisher.h"
#include "RequestReceiver.h"

namespace gringofts {
namespace demo {

class App final {
 public:
  explicit App(const char *configPath);
  ~App();

  // disallow copy ctor and copy assignment
  App(const App &) = delete;
  App &operator=(const App &) = delete;

  // disallow move ctor and move assignment
  App(App &&) = delete;
  App &operator=(App &&) = delete;

  void run();

  void shutdown();

 private:
  void initDeploymentMode(const INIReader &reader);

  void initMonitor(const INIReader &reader);

  void initCommandEventStore(const INIReader &reader);

  void startRequestReceiver();

  void startNetAdminServer();

  void startProcessCommandLoop();

  void startEventApplyLoop();

  void startPersistLoop();

  void startPostServerLoop();

 private:
  DeploymentMode mDeploymentMode = DeploymentMode::Standalone;

  BlockingQueue<std::shared_ptr<Command>> mCommandQueue;
  std::unique_ptr<RequestReceiver> mRequestReceiver;
  std::unique_ptr<app::CommandProcessLoopInterface> mCommandProcessLoop;
  std::shared_ptr<app::EventApplyLoopInterface> mEventApplyLoop;
  std::unique_ptr<app::NetAdminServer> mNetAdminServer;
  std::unique_ptr<BundleExposePublisher> mPostServer;

  std::shared_ptr<CommandEventStore> mCommandEventStore;
  std::unique_ptr<ReadonlyCommandEventStore> mReadonlyCommandEventStoreForCommandProcessLoop;
  std::unique_ptr<ReadonlyCommandEventStore> mReadonlyCommandEventStoreForEventApplyLoop;
  std::unique_ptr<ReadonlyCommandEventStore> mReadonlyCommandEventStoreForPostServer;

  std::shared_ptr<gringofts::CryptoUtil> mCrypto;

  std::thread mServerThread;
  std::thread mNetAdminServerThread;
  std::thread mCommandProcessLoopThread;
  std::thread mEventApplyLoopThread;
  std::thread mPersistLoopThread;
  std::thread mPostServerThread;

  bool mIsShutdown;
};

}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_APP_APP_H_

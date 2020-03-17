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

#ifndef SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_POST_BUNDLEEXPOSEPUBLISHER_H_
#define SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_POST_BUNDLEEXPOSEPUBLISHER_H_

#include <atomic>

#include <INIReader.h>

#include "../../../../infra/es/ReadonlyCommandEventStore.h"
#include "../../../../infra/util/TlsUtil.h"
#include "../../../generated/grpc/publishEvents.grpc.pb.h"

namespace gringofts {
namespace demo {

/**
 * publish command and event from command event store via grpc
 */
class BundleExposePublisher : public protos::BundleExposeService::Service {
 public:
  BundleExposePublisher(const INIReader &, std::unique_ptr<ReadonlyCommandEventStore>);
  ~BundleExposePublisher() override = default;

  grpc::Status FetchBundle(grpc::ServerContext *context,
                           const protos::FetchBundleRequest *request,
                           protos::FetchBundleResponse *response) override;

  grpc::Status FetchBundles(grpc::ServerContext *context,
                            const protos::FetchBundlesRequest *request,
                            protos::FetchBundlesResponse *response) override;

  void run();
  void shutdown();

 private:
  grpc::Status fetchBundlesImpl(grpc::ServerContext *context,
                                const protos::FetchBundlesRequest *request,
                                protos::FetchBundlesResponse *response);

  std::unique_ptr<ReadonlyCommandEventStore> mReadonlyCommandEventStore;
  int mPort;
  std::unique_ptr<grpc::Server> mServer;
  std::optional<TlsConf> mTlsConfOpt;

  /// throttle
  int64_t mMaxConcurrency = 10;
  std::atomic<uint64_t> mCurrentConcurrency = 0;
};

}  /// namespace demo
}  /// namespace gringofts

#endif  // SRC_APP_DEMO_SHOULD_BE_GENERATED_DOMAIN_POST_BUNDLEEXPOSEPUBLISHER_H_


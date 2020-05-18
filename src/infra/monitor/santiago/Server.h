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

#ifndef SRC_INFRA_MONITOR_SANTIAGO_SERVER_H_
#define SRC_INFRA_MONITOR_SANTIAGO_SERVER_H_

#include <memory>
#include <string>

#include <prometheus/exposer.h>

#include "MetricsCenter.h"

namespace prometheus {
class Exposer;
}

namespace santiago {
class Server {
 public:
  explicit Server(const std::string &address = "0.0.0.0", uint16_t port = 9091) :
      mExposerPtr(std::make_shared<::prometheus::Exposer>(address + ":" + std::to_string(port))) {}

  virtual ~Server() = default;
  inline void Registry(const santiago::MetricsCenter &center) {
    mExposerPtr->RegisterCollectable(center.getRegistryPtr());
  }
 private:
  std::shared_ptr<::prometheus::Exposer> mExposerPtr;
};

}  /// namespace santiago

#endif  // SRC_INFRA_MONITOR_SANTIAGO_SERVER_H_

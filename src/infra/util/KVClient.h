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

#ifndef SRC_INFRA_UTIL_KVCLIENT_H_
#define SRC_INFRA_UTIL_KVCLIENT_H_

#include "../util/TlsUtil.h"
#include <grpc++/grpc++.h>

namespace gringofts::kv {

using ::grpc::Channel;
using ::grpc::Status;
using KVPair = std::pair<std::string, std::string>;

/**
 * Client can fetch config from cloud
 */
struct Client {
  virtual ~Client() = default;
  virtual Status getPrefix(const std::string &prefix, std::vector<KVPair> *out) const = 0;
  virtual Status getValue(const std::string &prefix, std::string *out) const = 0;
};

/**
 * Client Factory can build a kv client
 */
class ClientFactory {
 public:
  virtual ~ClientFactory() = default;
  virtual std::shared_ptr<Client> produce(const INIReader &reader) const = 0;
};

/**
 * One Factory which provide endpoints and tls to build a client
 * @tparam T Client type
 * it must has a constructor with (std::vector<std::string>, std::optional<TlsConf>)
 */

std::tuple<std::vector<std::string>, std::optional<TlsConf>> parseINI(const INIReader &reader);

template<typename T>
class ClientFactory_t : public ClientFactory {
 public:
  std::shared_ptr<Client> produce(const INIReader &reader) const override {
    auto[endPoints, tls] = parseINI(reader);
    return std::make_shared<T>(std::move(endPoints), std::move(tls));
  }
};

}  // namespace gringofts::kv
#endif  // SRC_INFRA_UTIL_KVCLIENT_H_

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

#ifndef SRC_INFRA_UTIL_TLSUTIL_H_
#define SRC_INFRA_UTIL_TLSUTIL_H_

#include <optional>

#include <INIReader.h>
#include <grpc++/grpc++.h>
#include <grpc++/security/credentials.h>


namespace gringofts {

/**
 * Configurations for Transport Layer Security
 */
struct TlsConf {
  /// private key of https server
  std::string key;

  /// public key certificate of https server
  std::string cert;

  /// certificate authority
  /// used by https client to verify public key certificate of https server
  std::string ca;
};

/**
 * Utils for Transport Layer Security
 */
class TlsUtil final {
 public:
  /// parse TLS Conf from configure file
  static std::optional<TlsConf> parseTlsConf(const INIReader &iniReader, const std::string &section);

  /// Builds Server Credentials.
  static std::shared_ptr<grpc::ServerCredentials> buildServerCredentials(std::optional<TlsConf> tlsConfOpt);

  /// Build Channel Credentials
  static std::shared_ptr<grpc::ChannelCredentials> buildChannelCredentials(std::optional<TlsConf> tlsConfOpt);
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_TLSUTIL_H_

/************************************************************************
Copyright 2019-2021 eBay Inc.
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

#include "TlsUtil.h"


#include <spdlog/spdlog.h>

#include "FileUtil.h"

namespace gringofts {
std::optional<TlsConf> TlsUtil::parseTlsConf(const INIReader &iniReader,
                                           const std::string &section) {
  bool enable = iniReader.GetBoolean(section, "enable", false);
  if (!enable) {
    return std::nullopt;
  }

  TlsConf conf;

  std::string keyFile = iniReader.Get(section, "key.file", "");
  std::string certFile = iniReader.Get(section, "cert.file", "");
  std::string caFile = iniReader.Get(section, "ca.file", "");

  if (keyFile.empty() || certFile.empty() || caFile.empty()) {
    throw std::runtime_error("key.file or cert.file or ca.file is not set");
  }

  conf.key = FileUtil::getFileContent(keyFile);
  conf.cert = FileUtil::getFileContent(certFile);
  conf.ca = FileUtil::getFileContent(caFile);

  return conf;
}

/// Builds Server Credentials.
std::shared_ptr<grpc::ServerCredentials> TlsUtil::buildServerCredentials(std::optional<TlsConf> tlsConfOpt) {
  if (!tlsConfOpt) {
    /// no TLS Conf, use http
    SPDLOG_INFO("Server Side TLS disabled.");
    return grpc::InsecureServerCredentials();
  }

  const auto &tlsConf = *tlsConfOpt;
  grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp = {tlsConf.key, tlsConf.cert};

  grpc::SslServerCredentialsOptions sslOpts;
  sslOpts.pem_key_cert_pairs.push_back(pkcp);
  sslOpts.pem_root_certs = tlsConf.ca;

  SPDLOG_INFO("Server Side TLS enabled.");
  return grpc::SslServerCredentials(sslOpts);
}

/// Build Channel Credentials
std::shared_ptr<grpc::ChannelCredentials> TlsUtil::buildChannelCredentials(std::optional<TlsConf> tlsConfOpt) {
  if (!tlsConfOpt) {
    /// no TLS Conf, use http
    SPDLOG_INFO("Client Side TLS disabled.");
    return grpc::InsecureChannelCredentials();
  }

  const auto &tlsConf = *tlsConfOpt;

  grpc::SslCredentialsOptions sslOpts;
  sslOpts.pem_root_certs = tlsConf.ca;

  SPDLOG_INFO("Client Side TLS enabled.");
  return grpc::SslCredentials(sslOpts);
}
}   /// namespace gringofts

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

#ifndef SRC_INFRA_UTIL_TESSDNSRESOLVER_H_
#define SRC_INFRA_UTIL_TESSDNSRESOLVER_H_

#include <absl/strings/str_split.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <spdlog/spdlog.h>
#include <string>

#include "./DNSResolver.h"

namespace gringofts {

class TessDNSResolver : public DNSResolver{
 public:
  TessDNSResolver() = default;
  virtual ~TessDNSResolver() = default;

  std::string resolve(const std::string &hostname) override {
    /// TODO: https://jirap.corp.ebay.com/browse/RTCUTOFF-4576 the if code block can be deleted once the jira finished
    if (absl::StrContains(hostname, "tess.io")) {
      std::vector<std::string> subStrs = absl::StrSplit(hostname, ':');
      const auto *hostInfo = gethostbyname(subStrs.at(0).c_str());
      if (hostInfo == nullptr) {
        SPDLOG_INFO("failed to parse domain name {} after retry", hostname);
        return hostname;
      }
      assert(hostInfo != nullptr);
      const auto *ipStr = inet_ntoa(*((struct in_addr *)hostInfo->h_addr));
      SPDLOG_INFO("Successfully parse domain name from {} to {}", hostInfo->h_name, ipStr);
      return std::string(ipStr) + ":" + subStrs[1];
    } else {
      return hostname;
    }
  }
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_TESSDNSRESOLVER_H_

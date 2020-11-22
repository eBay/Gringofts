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

#ifndef SRC_INFRA_UTIL_DNSRESOLVER_H_
#define SRC_INFRA_UTIL_DNSRESOLVER_H_

#include <string>

namespace gringofts {

class DNSResolver {
 public:
  DNSResolver() = default;
  virtual ~DNSResolver() = default;

  virtual std::string resolve(const std::string &hostname) {
    /// the default implementation is simply returning the hostname
    return hostname;
  }
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_DNSRESOLVER_H_

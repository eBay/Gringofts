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
#include "KVClient.h"
#include <absl/strings/str_split.h>
#include <absl/strings/str_join.h>
#include <absl/strings/str_replace.h>

#include "../util/FileUtil.h"

namespace gringofts::kv {
constexpr const char *DELIM = ",";

std::tuple<std::vector<std::string>, std::optional<TlsConf>> parseINI(const INIReader &reader) {
  std::string ip_port = reader.Get("endpoints", "ip_port", "UNKNOWN");
  assert(ip_port != "UNKNOWN");
  // remove "\n" and "\"
  ip_port = absl::StrReplaceAll(ip_port, {{"\n", ""}, {"\\", ""}});
  std::vector<std::string> endPoints = absl::StrSplit(ip_port, DELIM);

  auto tlsEnable = reader.GetBoolean("tls", "enable", false);
  if (tlsEnable) {
    auto trustedCAFile = reader.Get("certs", "trusted_ca_file", "UNKNOWN");
    assert(trustedCAFile != "UNKNOWN");

    TlsConf conf;
    conf.ca = FileUtil::getFileContent(trustedCAFile);
    return {endPoints, conf};
  } else {
    return {endPoints, std::nullopt};
  }
}
}  // namespace gringofts::kv

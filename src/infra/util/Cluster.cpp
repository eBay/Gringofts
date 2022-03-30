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

#include "Cluster.h"

#include <absl/strings/str_format.h>

namespace gringofts {

std::string Node::to_string() const {
  auto str = mHostName + ":";
  return str;
}

std::string Cluster::to_string() const {
  std::string str;
  for (const auto &kv : mNodes) {
    auto idx = kv.first;
    str += absl::StrFormat("%d@%s,", idx, kv.second->to_string());
  }
  return str;
}

}  /// namespace gringofts


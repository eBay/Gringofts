/************************************************************************
Copyright 2021-2022 eBay Inc.
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

#ifndef SERVER_SRC_KV_ENGINE_UTILS_APPUTIL_H_
#define SERVER_SRC_KV_ENGINE_UTILS_APPUTIL_H_

#include <assert.h>
#include <boost/filesystem.hpp>
#include <netdb.h>
#include <string>
#include <unistd.h>

#include <spdlog/spdlog.h>

#include "StrUtil.h"

namespace goblin::kvengine::utils {

class AppUtil final {
 public:
  /**
   * get current version according to the current work directory
   * @return release version
   */
  static std::string getCurrentVersion() {
    namespace fs = boost::filesystem;
    std::string cwd = fs::current_path().string();
    SPDLOG_INFO("current work directory is {}", cwd);
    return getReleaseVersion(cwd);
  }

  static std::string getReleaseVersion(const std::string &cwd) {
    const char *suffix = "unx";
    const char *unknownVersion = "UNKNOWN";
    auto tokens = StrUtil::tokenize(cwd, '/');
    if (tokens.size() < 2) {
      return unknownVersion;
    }
    auto &version = tokens[tokens.size() - 2];
    return StrUtil::endsWith(version, suffix) ? version : unknownVersion;
  }
};

}  /// namespace goblin::kvengine::utils

#endif  // SERVER_SRC_KV_ENGINE_UTILS_APPUTIL_H_

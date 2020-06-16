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

#ifndef SRC_INFRA_RAFT_RAFTBUILDER_H_
#define SRC_INFRA_RAFT_RAFTBUILDER_H_

#include <memory>

#include <INIReader.h>

#include "RaftInterface.h"
#include "v2/RaftCore.h"

namespace gringofts {
namespace raft {

inline std::shared_ptr<RaftInterface> buildRaftImpl(const char *configPath, std::optional<std::string> clusterConfOpt) {
  INIReader iniReader(configPath);
  if (iniReader.ParseError() < 0) {
    SPDLOG_ERROR("Failed to load config file {}.", configPath);
    throw std::runtime_error("Failed to load config file");
  }

  auto version = iniReader.Get("raft.default", "version", "v2");
  SPDLOG_INFO("Build raft impl version {}.", version);

  if (version == "v2") {
    return std::make_shared<v2::RaftCore>(configPath, clusterConfOpt);
  } else {
    SPDLOG_ERROR("Unknown raft implement version {}.", version);
    exit(1);
  }
}

}  /// namespace raft
}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_RAFTBUILDER_H_

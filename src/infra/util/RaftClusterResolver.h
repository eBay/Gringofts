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

#ifndef SRC_INFRA_UTIL_RAFTCLUSTERRESOLVER_H_
#define SRC_INFRA_UTIL_RAFTCLUSTERRESOLVER_H_

#include <spdlog/spdlog.h>

#include "../etcdcli/EtcdKVClientTLS.h"

namespace gringofts {

class RaftClusterResolver final {
 public:
  /**
   * Return the raft cluster configuration
   */
  static std::optional<std::string> resolveRaftCluster(const INIReader &iniReader) {
    bool etcdEnabled = iniReader.GetBoolean("raft.etcd", "enable", false);
    if (!etcdEnabled) {
      return std::nullopt;
    }
    std::string etcdConfigFile = iniReader.Get("raft.etcd", "etcd.config.file", "");
    std::string clusterConfKey = iniReader.Get("raft.etcd", "cluster.conf.key", "");
    assert(!etcdConfigFile.empty() && !clusterConfKey.empty());

    /// init etcd client
    auto etcdCli = std::make_unique<EtcdKVClientTLS>(etcdConfigFile.c_str());
    /// read raft cluster conf from etcd
    auto clusterConf = etcdCli->getWithTLS(clusterConfKey);
    assert(clusterConfKey != "");

    return std::make_optional(clusterConf);
  }
};
}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_RAFTCLUSTERRESOLVER_H_

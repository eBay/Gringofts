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

#ifndef SRC_INFRA_RAFT_METRICS_RAFTMONITORADAPTOR_H_
#define SRC_INFRA_RAFT_METRICS_RAFTMONITORADAPTOR_H_

#include "../../monitor/Monitorable.h"
#include "../RaftInterface.h"

namespace gringofts {

using raft::RaftInterface;
using raft::RaftRole;

class RaftMonitorAdaptor : public Monitorable {
 public:
  explicit RaftMonitorAdaptor(const std::shared_ptr<RaftInterface> &server);
  virtual ~RaftMonitorAdaptor() = default;
 private:
  std::shared_ptr<RaftInterface> mServer;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_METRICS_RAFTMONITORADAPTOR_H_

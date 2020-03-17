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

#include "RaftMonitorAdaptor.h"

namespace gringofts {

RaftMonitorAdaptor::RaftMonitorAdaptor(const std::shared_ptr<RaftInterface> &server)
    : mServer(server) {
  // role
  MetricTag role_tag("current_role_v2", [this]() {
                       return static_cast<double>(mServer->getRaftRole());
                     },
                     {{"leader", std::to_string(static_cast<int>(RaftRole::Leader))},
                      {"candidate", std::to_string(static_cast<int>(RaftRole::Candidate))},
                      {"follower", std::to_string(static_cast<int>(RaftRole::Follower))}});
  mTags.push_back(std::move(role_tag));

  // term
  mTags.emplace_back("current_term_v2", [this]() {
    return mServer->getCurrentTerm();
  });

  // commit index
  mTags.emplace_back("committed_log_v2", [this]() {
    return mServer->getCommitIndex();
  });

  // last index
  mTags.emplace_back("last_index_v2", [this]() {
    return mServer->getLastLogIndex();
  });
}

}  /// namespace gringofts


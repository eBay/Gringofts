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

#ifndef SRC_INFRA_RAFT_RAFTSIGNAL_H_
#define SRC_INFRA_RAFT_RAFTSIGNAL_H_

#include "../util/Signal.h"
#include "RaftInterface.h"

namespace gringofts::raft {

class StopSyncRoleSignal : public Signal {
 public:
  explicit StopSyncRoleSignal(uint64_t index) : mBeginIndex(index) {}
  uint64_t getInitIndex() const { return mBeginIndex; }
 private:
  uint64_t mBeginIndex;
};

struct RaftState {
  uint64_t mFirstIndex;
  uint64_t mLastIndex;
  uint64_t mCommitIndex;
  RaftRole mRole;
};

class QuerySignal : public FutureSignal<RaftState> {};

}  // namespace gringofts::raft

#endif  // SRC_INFRA_RAFT_RAFTSIGNAL_H_

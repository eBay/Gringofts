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

#ifndef SRC_INFRA_ES_STORE_RAFTCOMMANDEVENTSTORE_H_
#define SRC_INFRA_ES_STORE_RAFTCOMMANDEVENTSTORE_H_

#include "../CommandEventStore.h"

#include "../../raft/RaftInterface.h"
#include "../../raft/RaftLogStore.h"
#include "../../raft/RaftReplyLoop.h"
#include "../../util/CryptoUtil.h"

namespace gringofts {

using raft::RaftInterface;
using raft::RaftLogStore;
using raft::RaftReplyLoop;
using raft::RaftRole;

class RaftCommandEventStore final : public CommandEventStore {
 public:
  RaftCommandEventStore(const std::shared_ptr<RaftInterface> &,
                        const std::shared_ptr<CryptoUtil> &);

  void persistAsync(const std::shared_ptr<Command> &,
                    const std::vector<std::shared_ptr<Event>> &,
                    uint64_t,
                    const std::string &) override;

  Transition detectTransition() override;
  std::optional<uint64_t> getLeaderHint() const override;

  uint64_t getCurrentTerm() const override { return mLastCheckedTerm; }

  void run() override {
    /// not implemented
  }

  void shutdown() override {
    /// not implemented
  }

 private:
  std::shared_ptr<RaftInterface> mRaftImpl;
  std::unique_ptr<RaftReplyLoop> mRaftReplyLoop;
  std::unique_ptr<RaftLogStore> mRaftLogStore;

  std::shared_ptr<CryptoUtil> mCrypto;

  uint64_t mLastCheckedTerm;
  RaftRole mLastCheckedRole;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_STORE_RAFTCOMMANDEVENTSTORE_H_

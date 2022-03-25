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

#ifndef SRC_INFRA_RAFT_RAFTINTERFACE_H_
#define SRC_INFRA_RAFT_RAFTINTERFACE_H_

#include <list>
#include <mutex>
#include <optional>

#include <spdlog/spdlog.h>

#include "generated/raft.pb.h"
#include "../grpc/RequestHandle.h"

namespace gringofts {
namespace raft {

using MemberId = uint64_t;
/// 0 should not be used as mId
constexpr MemberId kBadID = 0;

struct MemberInfo {
  MemberId mId = kBadID;
  /// host:port
  std::string mAddress;
  std::string toString() const {
    return std::to_string(mId) + "@" + mAddress;
  }
  bool operator < (const MemberInfo &other) const {
    return mId < other.mId;
  }
};

struct MemberOffsetInfo {
  MemberId mId = kBadID;
  /// host:port
  std::string mAddress;
  uint64_t mOffset;

  MemberOffsetInfo() = default;
  MemberOffsetInfo(MemberId id, std::string addr, uint64_t offset) :
       mId(id), mAddress(std::move(addr)), mOffset(offset) {}

  std::string toString() const {
    return std::to_string(mId) + "@" + mAddress;
  }
};

//////////////////////////// Client Request ////////////////////////////

struct ClientRequest {
  /// <index, term> is filled with <mLogStoreIndex, mLogStoreTerm>
  /// raft verifies the write to WAL by checking index and term
  LogEntry mEntry;

  /// Async Callback of Client
  RequestHandle *mRequestHandle = nullptr;
};

using ClientRequests = std::vector<ClientRequest>;

struct SyncFinishMeta {
  /// the begin index this cluster to process
  uint64_t mBeginIndex;
};

struct SyncRequest {
  /// <index, term> is filled with <mLogStoreIndex, mLogStoreTerm>
  /// raft verifies the write to WAL by checking index and term
  std::vector<raft::LogEntry> mEntries;

  uint64_t mStartIndex;
  uint64_t mEndIndex;

  /// meta indicate finish sync mode
  std::optional<SyncFinishMeta> mFinishMeta;
};

//////////////////////////// Raft Interface ////////////////////////////

enum class RaftRole {
  Leader = 0,
  Follower = 1,
  Candidate = 2,
  Syncer = 3
};

class RaftInterface {
 public:
  RaftInterface() = default;

  /// forbidden copy/move
  RaftInterface(const RaftInterface &) = delete;
  RaftInterface &operator=(const RaftInterface &) = delete;

  virtual ~RaftInterface() = default;

  /// kinds of getters
  virtual RaftRole getRaftRole() const = 0;
  virtual uint64_t getCommitIndex() const = 0;
  virtual uint64_t getCurrentTerm() const = 0;
  virtual uint64_t getFirstLogIndex() const = 0;
  /// for split, if cluster = 0, it return 0
  /// otherwise, it return the first index this cluster start process
  virtual uint64_t getBeginLogIndex() const = 0;
  virtual uint64_t getLastLogIndex() const = 0;
  virtual std::optional<uint64_t> getLeaderHint() const = 0;
  virtual std::vector<MemberInfo> getClusterMembers() const = 0;

  /// return leader commit index
  /// Using commit index instead of majority index as leader offset is due to one edge
  /// case for getting in-sync replica (ISR).
  /// Say we have a 5-member raft cluster, and use (majority index - match index) as
  /// each peer's lag for detecting slow followers. If two followers are in-sync for
  /// some time, and the other two are slow. Once the two in-sync followers crash,
  /// the majority index will turn much smaller. In that case, the two slower followers
  /// will start serve read requests before they catch up with the leader.
  virtual uint64_t getMemberOffsets(std::vector<MemberOffsetInfo> *) const = 0;

  /// used by StateMachine to read committed entry at index
  /// return true if succeed, return false if the entry is truncated.
  /// Attention that, index should be less than or equal to commitIndex.
  virtual bool getEntry(uint64_t index, LogEntry *entry) const = 0;

  /// used by StateMachine to read committed entries between
  /// [startIndex, startIndex + size - 1]. if everything is fine,
  /// return number of fetched entries. otherwise, return 0.
  virtual uint64_t getEntries(uint64_t startIndex,
                              uint64_t size,
                              std::vector<LogEntry> *entries) const = 0;

  /// used by RaftLogStore to send a batch of client requests
  virtual void enqueueClientRequests(ClientRequests clientRequests) = 0;

  /// using it to sync logs with others
  virtual void enqueueSyncRequest(SyncRequest syncRequest) {}

  /// used by NetAdminServer to do log retention.
  /// truncate log prefix from [firstIndex, lastIndex] to [firstIndexKept, lastIndex]
  /// Attention that, firstIndexKept should be less than or equal to commitIndex
  virtual void truncatePrefix(uint64_t firstIndexKept) = 0;
};

}  /// namespace raft
}  /// namespace gringofts

#endif  // SRC_INFRA_RAFT_RAFTINTERFACE_H_

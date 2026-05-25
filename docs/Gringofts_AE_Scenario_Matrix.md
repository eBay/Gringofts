# Gringofts Raft: AppendEntries Scenario Walkthrough

## Scope
This document focuses on the index-handling behavior of:
- `appendEntries()`
- `handleAppendEntriesRequest()`
- `handleAppendEntriesResponse()`

in `<Gringofts>/src/infra/raft/v2/RaftCore.cpp`.

This document covers retained-window overlap, rollback-reset, and truncate-boundary scenarios in these functions. It does not try to enumerate non-index control-flow branches such as term/role rejection, higher-term step-down, or duplicated-response ignore.

## Assumptions
- Unless stated otherwise, the RPC transport itself succeeds.
- Unless stated otherwise, there is no duplicated/delayed AE request or AE response.
- Unless stated otherwise, the request/response has already reached the index-handling path discussed below.

## Important implementation notes

Gringofts enhances raft:
- Gringofts explicitly handles the case where follower `lastLogIndex` rolls back to a value smaller than the leader's recorded `peer.matchIndex`. This can happen in real deployments if a follower loses its local Raft disk and restarts from empty (`lastLogIndex == 0`), or restarts from a backup whose `lastLogIndex` is smaller than the cached `peer.matchIndex`.
- Gringofts introduces `firstLogIndex`. Each node may delete already-applied historical Raft logs after the state machine has materialized the full system state produced by those logs. For disk-capacity reasons, `firstLogIndex` records the smallest log index still retained on that node, and different nodes may have different `firstLogIndex` values.

Variable meanings used throughout this document:
- `firstLogIndex`: the smallest log index still retained on a node. In the log API, its initial value is 1.
- `lastLogIndex`: the largest log index currently retained on a node. In the log API, its initial value is 0, which means the local log is empty.
- `peer.matchIndex`: on the leader, the largest entry index for which this follower is known to share the same log entries up to and including that index with the leader. Normally it monotonically increases within a term.
- `peer.nextIndex`: on the leader, the index of the next entry to send to the follower.
- `peer.preLogIndex`: on the leader, the index before `peer.nextIndex`.
- `commitIndex`: the committed boundary on the local node. In the raft interface it is the leader commit index, and the storage-layer comments treat entries up to this boundary as committed / immutable for state-machine reads.

Example on the leader side:

```text
                                   commitIndex
              firstLogIndex    peer.matchIndex  peer.nextIndex       lastLogIndex
                   |                      |       |                      |
leader raft log: [1000 ................. 1800   1801 .................. 2000]
```

In this example, the leader believes this follower already matches through index 1800, so the next AE for this follower starts from index 1801.

Abnormal cases handling:<br>
- For abnormal cases, the design principle is conservative recovery. 
- If the leader cannot find a retained matched log with the follower, it does not continue sending raft-log entries blindly. Instead, it suppresses bulk-data sending and keeps the peer in probe mode. Depending on the follower response, probing can restart from the leader retained tail or from `min(follower.lastLogIndex, leader.lastLogIndex) + 1`. If these probes still cannot establish a retained matched point, the follower stays in that probe-only state and `follower.lastLogIndex` does not grow by AE alone. In those abnormal shapes, manual recovery of follower data is required before normal raft-log replication can resume.

## Scenarios

This walkthrough covers these scenarios:
- Normal cases
- New leader cases
- Follower disk was lost cases
- Truncate cases

Marker meaning:
- `✅`: healthy paths
- `⚠️`: There is no coredump and no out-of-service, but at least one follower is in an abnormal state, for example it cannot catch up by AE alone.
- `❌`: This path can coredump or make the service out-of-service, so it is not an acceptable runtime path.

### Problematic index shapes summary

All problematic shapes in current code reduce to the following families:

- `peer.preLogIndex < leader.firstLogIndex - 1` and `peer.preLogIndex < follower.firstLogIndex`
  - The probe point is below both retained windows.
  - The leader can only send empty probes.
  - Successful probes only echo the stale probe point, so this peer stays pinned below both retained windows and AE alone cannot recover it. (`⚠️`)

- `peer.preLogIndex < leader.firstLogIndex` and `follower.firstLogIndex <= peer.preLogIndex <= follower.lastLogIndex`
  - The probe point is still retained on the follower but no longer retained on the leader.
  - The leader sends `request.prevLogTerm == 0`, while the follower still has a real retained term at that index, so the follower rejects.
  - If `follower.lastLogIndex < leader.firstLogIndex`, the reject resets this peer into the leader-tail retry state, and AE alone cannot rebuild retained overlap. (`⚠️`)
  - If `follower.lastLogIndex >= leader.firstLogIndex`, the reject resets probing to `peer.nextIndex = min(follower.lastLogIndex, leader.lastLogIndex) + 1`. When the follower retained tail is not itself a real matched point, later rejects can make the search alternate between the follower retained tail and another probe point below `leader.firstLogIndex`, so an older matched point inside the overlap window may still remain undiscovered. (`⚠️`)

- `follower.lastLogIndex < leader.firstLogIndex` and `peer.preLogIndex > follower.lastLogIndex`
  - There is no retained overlap, and the probe point is already to the right of the follower retained tail.
  - The follower rejects because `peer.preLogIndex > follower.lastLogIndex`.
  - For most members of this family, current code pushes the peer back toward a leader-tail retry state rather than rebuilding overlap by AE alone.
  - The main exception is `response.last_log_index() == 0 && leader.firstLogIndex == 1`: there `peer.nextIndex` becomes 1 and replication can resume from the beginning. Otherwise this family does not coredump, but AE alone still cannot re-establish overlap. (`⚠️`)

### leader and follower have overlapped raft log window

For retained-window overlap, use both relations together:
- `leader.firstLogIndex <= follower.lastLogIndex`
- `follower.firstLogIndex <= leader.lastLogIndex`

The leader-only shapes split into two different tails:
- left tail: `leader.firstLogIndex <= peer.preLogIndex < follower.firstLogIndex` (scenario 4)
- right tail: `follower.lastLogIndex < peer.preLogIndex <= leader.lastLogIndex` (scenario 5)

This shape-based classification is a good outer checklist, but when `peer.preLogIndex` is still inside the follower retained raft log window, the final accept/reject result also depends on whether `request.prevLogTerm` matches the follower term at that index.

#### 1. `peer.preLogIndex` is inside both leader and follower retained raft log windows

##### ✅ `peer.matchIndex == peer.nextIndex - 1`
This is the ordinary replication shape after the leader already knows a valid matched point inside both retained windows.
```text
                                                match next
                                                  |    |
follower raft log:              [105.............120]  |
leader raft log:          [100...................120  121 122 123 124 125]
                                                  |    |
                                                 120  121
```
One AE can continue normal replication from this shape. If the follower still matches the leader at `peer.preLogIndex`, the leader can immediately append the next entries.

##### ✅ `peer.matchIndex < peer.nextIndex - 1`
Leader has already reset this peer into probe mode, but the current probe point still lands inside both retained windows.
```text
                  match                              next
                    |                                  |
follower raft log:  |           [105.............120]  |
leader raft log:    |     [100...................120  121 122 123 124 125]
                    |                                  |
                    0                                 121
```
This shape is also healthy. A successful probe can advance `peer.matchIndex` toward `peer.preLogIndex`, and after one or a few successful rounds it returns to the no-gap normal replication shape above.

#### 2. `peer.preLogIndex` is below both leader and follower retained raft log windows

##### ⚠️ `peer.matchIndex == peer.nextIndex - 1 < leader.firstLogIndex - 1`
This follower lost local raft log data or stayed offline for a long time. The leader advanced and truncated the old prefix away, while the cached peer state still points entirely below both retained windows even though the follower has already recovered a newer retained suffix.
```text
                    match next
                      |    |
follower raft log:    |    |           [105.............120]
leader raft log:      |    |     [100...................120  121 122 123 124 125]
                      |    |
                      10   11
```
The leader cannot describe this `peer.preLogIndex` from its own retained log, so it can only send empty probes. The follower accepts those probes because `request.prevLogIndex < follower.firstLogIndex`, but the success response only echoes the same stale `peer.matchIndex`. The probe point stays pinned there, so AE alone cannot bring this follower back to normal replication under the current leader. A later leader election can reinitialize `peer.nextIndex` and `peer.matchIndex`, but this shape does not self-heal through the current AE exchange alone.

##### ⚠️ `peer.matchIndex < peer.nextIndex - 1 < leader.firstLogIndex - 1`
Follower data was rolled back or restored, the leader reset this peer into probe mode, and the current probe point is still below both retained windows.
```text
                    match    next
                      |       |
follower raft log:    |       |           [105.............120]
leader raft log:      |       |     [100...................120  121 122 123 124 125]
                      |       |
                      0       11
```
The first empty probe can advance `peer.matchIndex` up to `peer.preLogIndex`, but that only turns the peer state into the previous no-gap pinned case. So this shape does not coredump, but it still cannot recover by AE alone.

##### ✅ `peer.nextIndex == leader.firstLogIndex`
This is the retained-boundary edge case where `peer.preLogIndex == leader.firstLogIndex - 1`.
```text
                    match next
                      |    |
follower raft log:    |    |   [105.............120]
leader raft log:      |  [100...................120  121 122 123 124 125]
                      |    |
                      99  100
```
Unlike the previous two cases, the leader can still describe `peer.preLogIndex == leader.firstLogIndex - 1` consistently with `prevLogTerm == 0`, so this shape is recoverable. If `peer.mSuppressBulkData == false`, the same AE can carry entries starting from `leader.firstLogIndex`; otherwise the first round is an empty boundary probe, and the next successful AE resumes data replication. The follower accepts because this probe point is below its retained window, skips any already-truncated prefix entries, and then continues from the first overlapping retained entry.

#### 3. `peer.preLogIndex` is inside follower's retained raft log window but below leader's retained raft log window

##### ⚠️ `peer.matchIndex == peer.nextIndex - 1 < leader.firstLogIndex`
This is a stale no-gap state: the leader believes it already has a matched point, but that point is retained only on the follower side, leader has truncated its prefix raft log.
```text
                    match next
                       |   |
follower raft log: [9  10  11...........105.............120]
leader raft log:       |   |     [100...................120  121 122 123 124 125]
                       |   |
                       10  11
```
The follower rejects the AE because `request.prevLogTerm == 0` at `peer.preLogIndex`, while that index is still retained on the follower and therefore has a real retained term. The leader then resets into probe mode with `peer.matchIndex = 0`, `peer.nextIndex = min(follower.lastLogIndex, leader.lastLogIndex) + 1`, and `peer.mSuppressBulkData = true`. If the follower retained tail is itself a real matched point, the next empty tail probe succeeds and replication can resume from there. If that tail point is not matched, the tail probe rejects, the binary-decrease branch moves `peer.nextIndex` back below `leader.firstLogIndex`, and the next reject sends it to the follower tail again. With the numbers above, the search can alternate between `peer.preLogIndex = 120` and `peer.preLogIndex = 60` even though indices `100..119` contain a valid matched point. In that subcase `follower.lastLogIndex` does not grow by AE alone, and manual data recovery is required.

##### ⚠️ `peer.matchIndex < peer.nextIndex - 1 < leader.firstLogIndex`
The leader is already probing, and the current probe point is still in the follower-only left tail.
```text
                    match          next
                      |             |
follower raft log:    |     [9  10  11...........105.............120]
leader raft log:      |             |     [100...................120  121 122 123 124 125]
                      |             |
                      0             11
```
This case follows the same probe/reset pattern as the previous case. If the follower retained tail is not a real matched point, probing can keep alternating between the follower retained tail and another probe point below `leader.firstLogIndex`, so `follower.lastLogIndex` still cannot grow by AE alone.

#### 4. `peer.preLogIndex` is below follower's retained raft log window but inside leader's retained raft log window

##### ✅ `peer.matchIndex == peer.nextIndex - 1`
This shape appears when the follower truncated a longer prefix than the leader, but the leader's cached matched point still lies in the leader-only left tail.
```text
                         match next
                            |   |
follower raft log:          |   |   [110........120]
leader raft log:    [100........................120  121 122 123 124 125]
                            |   |
                           105 106
```
This shape is recoverable and does not enter an abnormal branch, but it is not guaranteed to finish in one AE. Because `request.prevLogIndex < follower.firstLogIndex`, the follower accepts the AE without needing the old prefix entry itself. Entries below `follower.firstLogIndex` are skipped, and from the first overlapping retained entry onward the follower either keeps matching entries or truncates its conflicting uncommitted suffix and appends the leader's entries. Depending on batching and whether bulk data is already enabled, multiple AEs may still be needed before the peer returns to the steady no-gap shape.

##### ✅ `peer.matchIndex < peer.nextIndex - 1`
The leader is probing, and the current probe point has already moved into the leader-only left tail below `follower.firstLogIndex`.
```text
                    match          next
                      |             |
follower raft log:    |             |   [110........120]
leader raft log:      |  [100......106..............120  121 122 123 124 125]
                      |             |
                      0            106
```
This shape is also healthy. If the current AE is an empty probe, one successful round lifts `peer.matchIndex` to `peer.preLogIndex`, and the next AE continues from the previous no-gap case. If bulk data is already enabled, the leader can continue replication in the same round.

#### 5. `peer.preLogIndex` is above follower's retained raft log window but inside leader's retained raft log window

##### ✅ `peer.matchIndex == peer.nextIndex - 1`
This shape appears when the leader still caches a matched point in a follower-lost suffix, while the follower has already rolled back to an older retained tail.
```text
                                                            match next
                                                               |   |
follower raft log:              [105.............120]          |   |
leader raft log:          [100...................120  121 122 123 124 125]
                                                               |   |
                                                              123 124
```
The follower rejects this AE because `peer.preLogIndex > follower.lastLogIndex`. Current code then treats it as follower rollback, resets `peer.matchIndex = 0`, turns on probe mode, and finally pulls `peer.nextIndex` back to `follower.lastLogIndex + 1`. So this shape does not get pinned here. After one failed AE, it falls back to an overlapped probe shape and continues searching for a matched point there.

##### ✅ `peer.matchIndex < peer.nextIndex - 1`
The leader is already probing, but the current probe point is still above the follower retained tail.
```text
                  match                                          next
                    |                                             |
follower raft log:  |           [105.............120]             |
leader raft log:    |     [100...................120 121 122 123 124 125]
                    |                                             |
                    0                                            124
```
This AE is also rejected because `peer.preLogIndex > follower.lastLogIndex`. Current code then reduces `peer.nextIndex` to `follower.lastLogIndex + 1`, so the next probe moves back into the retained overlap window and continues searching for a matched point there.

### leader and follower haven't overlapped raft log window, `follower.lastLogIndex == leader.firstLogIndex - 1`

For this boundary case, the follower retained tail touches the leader retained head, so there is no retained-window gap between them.

#### 1. `peer.preLogIndex` is below both leader and follower retained raft log windows

##### ⚠️ `peer.matchIndex == peer.nextIndex - 1`
This is the no-gap stale old-probe shape. A typical way to enter this scenario is that the follower stayed offline for a while, the leader truncated its old prefix during that time, and then the follower recovered its local data from a relatively new backup. The recovered follower is still behind the leader, but only by the retained-window boundary, so `follower.lastLogIndex == leader.firstLogIndex - 1` and the two retained windows still do not overlap.
```text
                    match next
                      |    |
follower raft log:    |    |      [90................99]
leader raft log:      |    |                            [100...............120 121 122 123 124 125]
                      |    |
                      10   11
```
The leader can only send empty probes from this shape. The follower accepts because `peer.preLogIndex < follower.firstLogIndex`, but the success response only echoes the same stale matched point. So this shape stays pinned below both retained windows and AE alone cannot recover it. The empty-follower edge case `follower.lastLogIndex == 0` and `leader.firstLogIndex == 1` is not this subcase, because there `peer.preLogIndex == leader.firstLogIndex - 1` and the leader can resume normal replication from index 1.

##### ⚠️ `peer.matchIndex < peer.nextIndex - 1`
The leader is still probing below both retained windows.
```text
                    match    next
                      |       |
follower raft log:    |       |    [90................99]
leader raft log:      |       |                          [100...............120 121 122 123 124 125]
                      |       |
                      0       11
```
One successful empty probe lifts `peer.matchIndex` to `peer.preLogIndex`, but that only turns the peer state into the previous no-gap pinned case. So this shape also cannot recover by AE alone.

#### 2. `peer.preLogIndex` is inside follower's retained raft log window

##### ⚠️ `peer.matchIndex == peer.nextIndex - 1`
The leader believes it already has a matched point in the follower-only retained window right below the leader boundary.
```text
                                      match next
                                        |    |
follower raft log:   [90................99]  |
leader raft log:                        |  [100...............120 121 122 123 124 125]
                                        |    |
                                        99  100
```
The follower rejects this AE because `peer.preLogIndex` is still retained on the follower, while the leader sends `request.prevLogTerm == 0`. Since `peer.nextIndex <= leader.firstLogIndex` and `response.lastLogIndex < leader.firstLogIndex`, the leader resets `peer.matchIndex = 0`, `peer.nextIndex = leader.lastLogIndex + 1`, and `peer.mSuppressBulkData = true`. Subsequent AE rounds stay in the leader-tail retry pattern, so AE alone cannot re-establish retained overlap.

##### ⚠️ `peer.matchIndex < peer.nextIndex - 1`
The leader is already probing, but the current probe point still lies in the follower-only retained window.
```text
                    match                        next
                      |                           |
follower raft log:    |   [90................99]  |
leader raft log:      |                         [100...............120 121 122 123 124 125]
                      |                           |
                      0                          100
```
The follower still rejects because `request.prevLogTerm == 0` does not match the retained follower term at `peer.preLogIndex`. The same guard/reset path sends this peer directly into the leader-tail retry state, so subsequent AE rounds keep probing from the leader side and wait for manual recovery before retained overlap can exist again.

#### 3. `peer.preLogIndex` is inside leader's retained raft log window

##### ⚠️ `peer.matchIndex == peer.nextIndex - 1`
The leader still caches a matched point in a follower-lost suffix, while the follower retained tail has already stopped just below the leader retained head.
```text
                                                                            match next
                                                                               |   |
follower raft log:       [90................99]                                |   |
leader raft log:                               [100...............120 121 122 123 124 125]
                                                                               |   |
                                                                              123 124
```
The follower rejects because `peer.preLogIndex > follower.lastLogIndex`. Current code then treats it as follower rollback, resets `peer.matchIndex = 0`, and finally leaves this peer in the leader-tail retry state. So this shape does not coredump, but it also does not recover by AE alone.

##### ⚠️ `peer.matchIndex < peer.nextIndex - 1`
The leader is already probing, but the current probe point already lies in the leader-only right tail.
```text
                    match                                                         next
                      |                                                             |
follower raft log:    |   [90................99]                                    |
leader raft log:      |                         [100...............120 121 122 123 124 125]
                      |                                                             |
                      0                                                            124
```
The follower rejects because `peer.preLogIndex > follower.lastLogIndex`. Current code responds to `response.lastLogIndex < leader.firstLogIndex` by pushing this peer into the same leader-tail retry state.

### leader and follower haven't overlapped raft log window, `follower.lastLogIndex < leader.firstLogIndex - 1`

For this deeper-gap case, there is a real retained-window gap between the follower retained tail and the leader retained head.

#### 1. `peer.preLogIndex` is below both leader and follower retained raft log windows

##### ⚠️ `peer.matchIndex == peer.nextIndex - 1`
This is the no-gap stale old-probe shape.
```text
                    match next
                      |    |
follower raft log:    |    |      [90.........95]
leader raft log:      |    |                            [100...............120 121 122 123 124 125]
                      |    |
                      10   11
```
The leader again sends empty probes. The follower accepts because `peer.preLogIndex < follower.firstLogIndex`, but the success response only echoes the same stale matched point. So this shape stays pinned below both retained windows.

##### ⚠️ `peer.matchIndex < peer.nextIndex - 1`
The leader is still probing below both retained windows.
```text
                    match    next
                      |       |
follower raft log:    |       |   [90............95]
leader raft log:      |       |                            [100...............120 121 122 123 124 125]
                      |       |
                      0       11
```
One successful empty probe lifts `peer.matchIndex` to `peer.preLogIndex`, but that only turns the peer state into the previous no-gap pinned case. So this shape is still pinned below both retained windows.

#### 2. `peer.preLogIndex` is inside follower's retained raft log window

##### ⚠️ `peer.matchIndex == peer.nextIndex - 1`
The leader still caches a matched point inside the follower-only retained window, even though the follower window is now separated from the leader window by a real gap. A typical way to enter this scenario is that the follower stayed offline for a while, the leader truncated its old prefix during that time, and then the follower came back. This scenario is almost same as scenario 3.
```text
                                 match next
                                    |   |
follower raft log:   [90............95] |
leader raft log:                    |   |      [100.......120 121 122 123 124 125]
                                    |   |
                                    95  96
```
The follower rejects because `request.prevLogTerm == 0` does not match the retained follower term at `peer.preLogIndex`. Since `peer.nextIndex <= leader.firstLogIndex` and `response.lastLogIndex < leader.firstLogIndex`, the leader resets `peer.matchIndex = 0`, `peer.nextIndex = leader.lastLogIndex + 1`, and `peer.mSuppressBulkData = true`. Subsequent AE rounds stay in the leader-tail retry pattern, so AE alone cannot create retained overlap.

##### ⚠️ `peer.matchIndex < peer.nextIndex - 1`
The leader is probing, but the current probe point is still inside the follower-only retained window.
```text
                    match                   next
                      |                      |
follower raft log:    |   [90............95] |
leader raft log:      |                      |     [100.......120 121 122 123 124 125]
                      |                      |
                      0                      96
```
The follower still rejects because `request.prevLogTerm == 0` does not match the retained follower term at `peer.preLogIndex`. The same guard/reset path sends this peer directly into the leader-tail retry state.

#### 3. `peer.preLogIndex` is between follower and leader retained raft log windows

##### ⚠️ `peer.matchIndex == peer.nextIndex - 1`
The leader believes it already has a matched point in the retained-window gap between follower and leader.
```text
                                         match next
                                            |   |
follower raft log:    [90............95]    |   |       
leader raft log:                            | [100.......120 121 122 123 124 125]
                                            |   |
                                            99 100
```
The follower rejects because `peer.preLogIndex > follower.lastLogIndex`. Current code then treats it as follower rollback, resets `peer.matchIndex = 0`, and leaves this peer in the leader-tail retry state. So this shape does not coredump, but it also does not recover by AE alone.

##### ⚠️ `peer.matchIndex < peer.nextIndex - 1`
The leader is probing, and the current probe point lies in the retained-window gap between follower and leader. This includes the edge `peer.preLogIndex == leader.firstLogIndex - 1`.
```text
                    match                               next
                      |                                  |
follower raft log:    |       [90............95]         |
leader raft log:      |                                [100...............120 121 122 123 124 125]
                      |                                  |
                      0                                 100
```
The follower rejects because `peer.preLogIndex > follower.lastLogIndex`. Even though this point is not retained on either side, `response.lastLogIndex < leader.firstLogIndex` still pushes this peer into the same leader-tail retry state.

#### 4. `peer.preLogIndex` is inside leader's retained raft log window

##### ⚠️ `peer.matchIndex == peer.nextIndex - 1`
The leader still caches a matched point in a follower-lost suffix while the follower retained window remains separated below by a real gap.
```text
                                                                            match next
                                                                               |   |
follower raft log:       [90.........95]                                       |   |
leader raft log:                               [100...............120 121 122 123 124 125]
                                                                               |   |
                                                                              123 124
```
The follower rejects because `peer.preLogIndex > follower.lastLogIndex`. Current code then treats it as follower rollback, resets `peer.matchIndex = 0`, and leaves this peer in the leader-tail retry state.

##### ⚠️ `peer.matchIndex < peer.nextIndex - 1`
The leader is already probing, but the current probe point already lies in the leader-only right tail.
```text
                    match                                                         next
                      |                                                             |
follower raft log:    |   [90.........95]                                           |
leader raft log:      |                         [100...............120 121 122 123 124 125]
                      |                                                             |
                      0                                                            124
```
The follower rejects because `peer.preLogIndex > follower.lastLogIndex`. As in the previous subcase, current code responds to `response.lastLogIndex < leader.firstLogIndex` by keeping this peer in the leader-tail retry state.

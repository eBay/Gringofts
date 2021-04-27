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

#ifndef SRC_INFRA_ES_COMMANDEVENTSTORE_H_
#define SRC_INFRA_ES_COMMANDEVENTSTORE_H_

#include "../common_types.h"
#include "Command.h"
#include "CommandDecoder.h"
#include "Event.h"
#include "EventDecoder.h"

namespace gringofts {

// forward declaration
enum class Transition;
/**
 * CommandEventStore persists commands and events into the persistence layer,
 * be it RDBMS, NoSQL or distribution log.
 *
 * If you are from the world of event-sourcing, it's called Repository. They have
 * the same responsibilities.
 *
 * Discussion #1: Why a store for both commands and events?
 * This is only logical, under the hood, there will likely be one dedicated
 * physical store for each.
 *
 * The main reason we want to have this logical store, with interfaces accepting
 * both commands and events, is we need the persist operation to be semi-transactional,
 * i.e., the fact that events being persisted guarantees the command that generated
 * these events must have been persisted, however the other way round is not always true.
 *
 * Doing so gives us several advantages:
 *
 * The biggest advantage is recover after application crash is largely
 * transparent to upstream systems.
 * 1. When application recovers after crash, it first replays all the events to
 * restore to the state as of the point it crashed, and then it uses the last applied
 * event to locate the last processed command, and starts processing command after it.
 * So as long as commands are persisted, events can be generated.
 * 2. On the other hand, if events are persisted while the command is not, not only it
 * now requires upstream to resend the request (thus not so transparent), but also
 * the logic to resolve the last processed command becomes unnecessarily complicated.
 *
 * The second advantage is it allows for optimizations cross commands and events now.
 * For example, if underlying persistence is distributed log backed by consensus group
 * such as RAFT, by putting commands and events in one interface, we can send them
 * in batches (although they may still be persisted into different distributed logs)
 * to improve overall throughput.
 */
class CommandEventStore {
 public:
  CommandEventStore() = default;
  virtual ~CommandEventStore() = default;

  /// forbidden copy and move
  CommandEventStore(const CommandEventStore &) = delete;
  CommandEventStore &operator=(const CommandEventStore &) = delete;

  /**
   * Persist command and the related events which are the output by calling the former's
   * processCommand method (See #gringofts::ProcessCommandStateMachine.processCommand).
   * Contract for any subclasses that implement this interface:
   * command/events persisting is transactional. In other words,
   * commands and events are either both persisted or none of them are persisted
   * @param command  : a command
   * @param events   : the events by calling \p command's processCommand
   * @param code     : return code
   * @param message  : return message
   */
  virtual void persistAsync(const std::shared_ptr<Command> &command,
                            const std::vector<std::shared_ptr<Event>> &events,
                            uint64_t code,
                            const std::string &message) = 0;

  /**
   * Upper layer will call this method to detect the underlying persistence's role transition, based on what
   * upper layer will switch the logic accordingly.
   * For those stores which are used when #gringofts::DeploymentMode is #gringofts::DeploymentMode::Distributed.
   * @return the transition result
   */
  virtual Transition detectTransition() { throw std::runtime_error("Not supported"); }
  /**
   * Upper layer will call this method to find out who is the leader
   */
  virtual std::optional<uint64_t> getLeaderHint() const { throw std::runtime_error("Not supported"); }

  /**
   * Consensus protocol has its own keyword about time:
   * for raft, it is term, for zab, it is epoch.
   * Return term of below store, if it is based consensus.
   */
  virtual uint64_t getCurrentTerm() const { throw std::runtime_error("Not supported"); }

  /**
   * Kick off the store to start persisting commands and events if there are any.
   *
   * This method should be invoked only once during application's lifetime. Successive calls will be simply ignored.
   *
   * Do NOT call this method in the main thread as it'll block till the application ends. Create a dedicated thread
   * and call it there.
   */
  virtual void run() = 0;

  /**
   * Shut down the store. ::gringofts::CommandEventStore::run will block until all pending commands and events
   * are persisted.
   *
   * After this method is called, calling ::gringofts::CommandEventStore::run will have no effect.
   *
   * This method should be invoked only once during application's lifetime. Successive calls will be simply ignored.
   */
  virtual void shutdown() = 0;

  // @formatter:off
  using CommandEventsEntry = std::tuple<std::shared_ptr<Command>, std::vector<std::shared_ptr<Event>>>;
  // @formatter:on
  using CommandEventQueue = BlockingQueue<CommandEventsEntry>;
};

enum class Transition {
  SameLeader = 0,
  SameFollower = 1,
  LeaderToFollower = 2,
  FollowerToLeader = 3,
  OldLeaderToNewLeader = 4,
  OldFollowerToNewFollower = 5
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_COMMANDEVENTSTORE_H_

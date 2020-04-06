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

#ifndef SRC_INFRA_ES_STATEMACHINE_H_
#define SRC_INFRA_ES_STATEMACHINE_H_

#include "CommandDecoder.h"
#include "Crypto.h"
#include "Event.h"
#include "EventDecoder.h"

namespace gringofts {

/**
 * A state machine keeps the internal state of an application. Its state can
 * only be updated by applying events (see #gringofts::Event).
 * This is the key to why the state can be fully restored by replaying all the
 * events persisted in #gringofts::CommandEventStore.
 */
class StateMachine {
 public:
  virtual ~StateMachine() = default;

  /**
   * Apply an event to the state machine, whose internal state may or may not
   * be updated.
   *
   * Determinism must be guaranteed by any sub class implementing
   * this function, i.e., if State_A + Event_1 -> State_B, then no matter
   * how many times this function is invoked, given State_A and Event_1,
   * the result state must always be State_B.
   * In other words, randomness is not allowed in this function.
   *
   * However, note that idempotency is not mandatory.
   * For example, suppose the logic is send notification when Event_1 happens,
   * depending on the business requirement, sub class can choose to send the
   * notification only once no matter how many times Event_1 is applied or
   * multiple times.
   *
   * Discussion #1: Can this function create more events?
   * In reality, it's quite common sense that one event can trigger more events
   * (see https://en.wikipedia.org/wiki/Butterfly_effect). However in current
   * design we choose a simplified model, as illustrated below:
   *
   *     Command -----(process)-----> Events -----(apply)-----> State
   *        ^                                                     |
   *        |_____________________________________________________|
   *
   * This model can satisfy most use scenarios and most importantly, it gives us
   * below advantages:
   * 1. 100% reproducibility, which is not possible in real world but
   * super critical for computer systems.
   * 2. Simplified programming model and architecture.
   *
   * So unless we can be convinced by a real use scenario which cannot be
   * handled in the current model, and it's deterministic that Event_1 always
   * leads to Event_2, we will not add extra (potentially large) effort now to
   * carefully handle the complexity introduced to meet 100% reproducibility.
   *
   * @param event an #gringofts::Event object to be applied
   * @return the state machine with the latest state
   */
  virtual StateMachine &applyEvent(const Event &event) = 0;

  /**
   * Caller invoke commit() to inform StateMachine that
   * all updates for appliedIndex have been applied.
   */
  virtual void commit(uint64_t appliedIndex) {}

  /**
   * Swap state with other state machine.
   */
  virtual void swapState(StateMachine *other) = 0;

  /**
   * Clear state of state machine.
   */
  virtual void clearState() = 0;

  /**
   * Check if two state machine has exactly the same state
   */
  virtual bool hasSameState(const StateMachine &) const = 0;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_STATEMACHINE_H_

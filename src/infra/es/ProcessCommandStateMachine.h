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

#ifndef SRC_INFRA_ES_PROCESSCOMMANDSTATEMACHINE_H_
#define SRC_INFRA_ES_PROCESSCOMMANDSTATEMACHINE_H_

#include "StateMachine.h"
#include "Command.h"

namespace gringofts {

struct ProcessHint {
  /**
   * The code to indicate the result of the process
   */
  uint64_t mCode;
  /**
   * A human-readable message that gives more details on the process result
   */
  std::string mMessage;
};

/**
 * A #gringofts::StateMachine that can process #gringofts::Command.
 */
class ProcessCommandStateMachine : public StateMachine {
 public:
  virtual ~ProcessCommandStateMachine() = default;

  /**
   * Execute the command to get 0, 1 or multiple events.
   *
   * This is the ONLY function that can produce events. See the detailed
   * discussion in #gringofts::StateMachine.
   *
   * This function is not guaranteed to be deterministic, i.e., the resulting
   * events can be totally different in every call, even the state
   * (see #gringofts::StateMachine) before every call is the same. In other
   * words, randomness IS allowed.
   * This is the only reason we cannot rely on this function to recover state.
   *
   * A few explanation on the return type:
   * - #gringofts::Event is guaranteed to be immutable, so const keyword is not
   *    needed here.
   * - Recommended way to use the method:
   *
   * ```
   * List<shared_ptr<Event>> events;
   * auto status = StateMachine.processCommand(command, &events);
   * ```
   *
   * > The language has special-case support for this: if you initialize
   * > a const T& with a temporary T, that T (in this case string) is not
   * > destroyed until the reference goes out of scope
   * > (in the common case of automatic or static variables).
   * More details are in https://abseil.io/tips/101.
   *
   * @param command the command to be executed. This method is part of
   * StateMachine because the execution of the command may need the latest
   * state, e.g., in FAS, when executing transfer asset command, the logic needs
   * to first check if the corresponding account has enough asset.
   * @param events a list of events as the output of processing the command. It can be 0, 1 or multiple events.
   * @return result of the process
   */
  virtual ProcessHint processCommand(
      const Command &command, std::vector<std::shared_ptr<Event>> *events) const = 0;

  /**
   * An aggregate method which first process the command, and then apply the resulting events.
   * @param command
   * @param events a list of events as the output of processing the command. It can be 0, 1 or multiple events.
   * @return result of the process
   */
  virtual ProcessHint processCommandAndApply(
      const Command &command, std::vector<std::shared_ptr<Event>> *events) = 0;

  /**
   * Update metrics which are exposed to external for monitoring purpose
   */
  virtual void updateMetrics() const {}
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_PROCESSCOMMANDSTATEMACHINE_H_

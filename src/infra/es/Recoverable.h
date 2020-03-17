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

#ifndef SRC_INFRA_ES_RECOVERABLE_H_
#define SRC_INFRA_ES_RECOVERABLE_H_

namespace gringofts {

class Recoverable {
 public:
  virtual ~Recoverable() = default;

  /**
   * Restore associated #gringofts::StateMachine's state to the point when last time it exited or crashed.
   *
   * One recommended restore procedure:
   *
   * 1. Replay events that have been persisted in the event store.
   * 2. Process commands that have been persisted in the command store but have not yet been handled.
   *
   * This method should be invoked only once at application startup.
   * A second call should exit the application with error.
   */
  virtual void recoverOnce() = 0;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_RECOVERABLE_H_

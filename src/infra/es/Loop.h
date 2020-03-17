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

#ifndef SRC_INFRA_ES_LOOP_H_
#define SRC_INFRA_ES_LOOP_H_

#include <stdexcept>

namespace gringofts {

class Loop {
 public:
  virtual ~Loop() = default;

  /**
   * Kick off the loop. It's a blocking call, so recommended to trigger this method in a separate thread.
   * This method can be called again after loop is shut down.
   * However, if this method is called multiple times in different threads, the behavior is undefined, and it's
   * strongly not recommended to do so.
   */
  virtual void run() {
    throw std::runtime_error("Not supported, probably deployment mode is distributed!");
  }

  /**
   * Comparing with #gringofts::Loop.run(), this method will be kicked off when
   * deployment mode is #gringofts::DeploymentMode::Distributed
   */
  virtual void runDistributed() {
    throw std::runtime_error("Not supported, probably deployment mode is standalone!");
  }

  /**
   * Ask the loop to shut down itself. Please note that the loop may still be running when this method returns. So
   * some kind of wait-and-check mechanism is needed to verify the loop is actually down.
   * This method can be called multiple times but not recommended to do so.
   *
   * Recommended call sequence:
   *
   *     run -----> shutdown -----> (verify loop is actually down)
   *        ^                                    |
   *        |____________________________________|
   */
  virtual void shutdown() = 0;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_LOOP_H_

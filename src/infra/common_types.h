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

#ifndef SRC_INFRA_COMMON_TYPES_H_
#define SRC_INFRA_COMMON_TYPES_H_

#include "mpscqueue/MpscDoubleBufferQueue.h"

namespace gringofts {

template<class T>
using BlockingQueue = MpscDoubleBufferQueue<T>;

#ifdef MAC_OS
using Id = u_long;
#else
using Id = ulong;
#endif
using Type = ::std::int8_t;

#ifdef MAC_OS
#define pthread_setname_np(_self_, _name_) ::pthread_setname_np(_name_);
#else
#define pthread_setname_np(_self_, _name_) ::pthread_setname_np(_self_, _name_);
#endif

/**
 * Determine how application will be deployed, it also affects the threading model and thread behavior.
 */
enum class DeploymentMode {
  /**
   * Application will be deployed as a single instance.
   * Use this mode if the persistence layer is #gringofts::DefaultCommandEventStore or
   * #gringofts::SQLiteCommandEventStore.
   */
  Standalone = 0,
  /**
   * Application will be deployed as a member of a cluster.
   * Use this mode if the persistence layer is RaftCommandEventStore.
   */
  Distributed = 1
};

}  /// namespace gringofts

#endif  // SRC_INFRA_COMMON_TYPES_H_

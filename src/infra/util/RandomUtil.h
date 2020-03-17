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

#ifndef SRC_INFRA_UTIL_RANDOMUTIL_H_
#define SRC_INFRA_UTIL_RANDOMUTIL_H_

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

namespace gringofts {

/**
 * Util class for random
 */
class RandomUtil final {
 public:
  /// Return a random num within [start, end]
  static uint64_t randomRange(uint64_t start, uint64_t end) {
    assert(start <= end);
    return start + ::rand() % (end - start + 1);  // NOLINT(runtime/threadsafe_fn)
  }
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_RANDOMUTIL_H_

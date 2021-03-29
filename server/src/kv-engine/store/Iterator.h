/************************************************************************
Copyright 2021-2022 eBay Inc.
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

#ifndef SERVER_SRC_KV_ENGINE_STORE_ITERATOR_H_
#define SERVER_SRC_KV_ENGINE_STORE_ITERATOR_H_

#include <string>

#include "../utils/Status.h"

namespace goblin::kvengine::store {

template<typename T>
class Iterator {
 public:
  Iterator() = default;
  virtual ~Iterator() = default;

  virtual utils::Status seekToBegin() = 0;
  virtual utils::Status next() = 0;
  virtual utils::Status prev() = 0;
  virtual utils::Status get(T &) = 0;
  virtual bool hasValue() = 0;
};

}  /// namespace goblin::kvengine::store

#endif  // SERVER_SRC_KV_ENGINE_STORE_ITERATOR_H_


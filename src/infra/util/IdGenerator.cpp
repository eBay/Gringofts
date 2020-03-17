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

#include "IdGenerator.h"

namespace gringofts {

IdGenerator::IdGenerator() : mCurrentId(0) {}

void IdGenerator::setCurrentId(const Id id) {
  if (id < mCurrentId) {
    throw std::runtime_error("Trying to set the current id to a smaller value which is disallowed.");
  }
  mCurrentId = id;
}

}  /// namespace gringofts

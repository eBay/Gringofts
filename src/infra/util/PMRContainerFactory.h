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

#ifndef SRC_INFRA_UTIL_PMRCONTAINERFACTORY_H_
#define SRC_INFRA_UTIL_PMRCONTAINERFACTORY_H_

#include "MemoryPool.h"

namespace gringofts {
class PMRContainerFactory {
 public:
  PMRContainerFactory(
      const std::string &name,
      std::unique_ptr<PMRMemoryPool> memoryPool):
    mName(name),
    mMemoryPool(std::move(memoryPool)) {
  }
  ~PMRContainerFactory() {
    mMemoryPool->release();
  }
  void resetPool() {
    mMemoryPool->reset();
  }
  template <class T, class... Args>
  T* newContainer(Args&&... args) {
    auto memoryResource = mMemoryPool->getMemoryResource();
    /// this will allocate the pointer and all internal data structures using the allocator
    /// 1. allocate memory
    auto ptr = memoryResource->allocate(sizeof(T), alignof(T));
    /// 2. invoke constructor and pass the allocator to it
    return ::new(static_cast<void*>(ptr)) T(std::forward<Args>(args)..., memoryResource);
  }
  template <class T>
  void deleteContainer(T* ptr) {
    auto memoryResource = mMemoryPool->getMemoryResource();
    /// this will de-allocate the pointer and all internal data structures using the allocator
    /// 1. invoke destructor
    ptr->~T();
    /// 2. deallocate memory
    memoryResource->deallocate(ptr, sizeof(T), alignof(T));
  }

 private:
  std::string mName;
  std::unique_ptr<PMRMemoryPool> mMemoryPool;
};
}  // namespace gringofts

#endif  // SRC_INFRA_UTIL_PMRCONTAINERFACTORY_H_

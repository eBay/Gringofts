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

#ifndef SRC_INFRA_UTIL_MEMORYPOOL_H_
#define SRC_INFRA_UTIL_MEMORYPOOL_H_

#include <map>
#include <memory>
#include <memory_resource>
#include <string>
#include <unistd.h>

namespace gringofts {

class PMRMemoryPool {
 public:
  explicit PMRMemoryPool(const std::string &name): mName(name) {}
  virtual ~PMRMemoryPool() = default;

  virtual void reset() {}
  virtual void release() {}

  std::pmr::memory_resource* getMemoryResource() {
    return mMemoryResource.get();
  }

 protected:
  std::string mName;
  std::unique_ptr<std::pmr::memory_resource> mMemoryResource;
};

class NewDeleteMemoryPool: public PMRMemoryPool {
 public:
  explicit NewDeleteMemoryPool(const std::string &name);
  virtual ~NewDeleteMemoryPool() = default;

 private:
  std::unique_ptr<std::pmr::memory_resource> mInternalMemoryResource;
};

class MonotonicPMRMemoryPool: public PMRMemoryPool {
 public:
  MonotonicPMRMemoryPool(const std::string &name, uint64_t reserveSizeInMB);
  virtual ~MonotonicPMRMemoryPool() = default;

  void reset() override;
  void release() override;

 private:
  void init();

  char* mBuffer;
  uint64_t mReserveSize = 0;
  std::unique_ptr<std::pmr::memory_resource> mInternalMemoryResource;
};
}  // namespace gringofts

#endif  // SRC_INFRA_UTIL_MEMORYPOOL_H_

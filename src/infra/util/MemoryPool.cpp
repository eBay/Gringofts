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

#include <spdlog/spdlog.h>

#include "MemoryPool.h"

#include "TimeUtil.h"
#include "TrackingMemoryResource.h"

namespace gringofts {

NewDeleteMemoryPool::NewDeleteMemoryPool(const std::string &name):
  PMRMemoryPool(name) {
  mMemoryResource = std::make_unique<TrackingMemoryResource>(name, std::pmr::new_delete_resource());
}

MonotonicPMRMemoryPool::MonotonicPMRMemoryPool(
    const std::string &name,
    uint64_t reserveSizeInMB):
  PMRMemoryPool(name), mReserveSize((uint64_t)1024 * 1024 * reserveSizeInMB) {
  auto start = gringofts::TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("pool {}: init memory pool with size {}", mName, mReserveSize);
  mBuffer = new char[mReserveSize]();
  init();
  auto end = gringofts::TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("pool {}: finish pool initialization, time cost {} ms", mName, (end - start) / 1000000.0);
}

void MonotonicPMRMemoryPool::reset() {
  init();
}

void MonotonicPMRMemoryPool::init() {
  SPDLOG_INFO("pool {}: init memory pool", mName);
  mInternalMemoryResource = std::make_unique<std::pmr::monotonic_buffer_resource>(
      mBuffer, mReserveSize, std::pmr::null_memory_resource());
  mMemoryResource = std::make_unique<TrackingMemoryResource>(mName, mInternalMemoryResource.get());
}

void MonotonicPMRMemoryPool::release() {
  auto start = gringofts::TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("releasing memory pool");
  mMemoryResource = nullptr;
  mInternalMemoryResource = nullptr;
  delete[] mBuffer;
  mReserveSize = 0;
  auto end = gringofts::TimeUtil::currentTimeInNanos();
  SPDLOG_INFO("pool {}: finish pool release, time cost {} ms", mName, (end - start) / 1000000.0);
}
}  // namespace gringofts

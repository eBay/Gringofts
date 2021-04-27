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

#include "TrackingMemoryResource.h"
#include "TimeUtil.h"

namespace gringofts {

TrackingMemoryResource::TrackingMemoryResource(const std::string &name, std::pmr::memory_resource* upstream):
  mName(name), mUpStream(upstream) {
  }

void* TrackingMemoryResource::do_allocate(size_t bytes, size_t alignment) {
  mCurMemUsage += bytes;
  uint64_t diff = (mCurMemUsage > mLastReportMemUsage) ?
    (mCurMemUsage - mLastReportMemUsage): (mLastReportMemUsage - mCurMemUsage);
  if (diff > kReportInterval) {
    SPDLOG_WARN("resource {}: mem usage {}, allocate {}",
        mName, mCurMemUsage, bytes);
    mLastReportMemUsage = mCurMemUsage;
  }
  void* ret = mUpStream->allocate(bytes, alignment);
  return ret;
}

void TrackingMemoryResource::do_deallocate(void* ptr, size_t bytes, size_t alignment) {
  mCurMemUsage -= bytes;
  uint64_t diff = (mCurMemUsage > mLastReportMemUsage) ?
    (mCurMemUsage - mLastReportMemUsage): (mLastReportMemUsage - mCurMemUsage);
  if (diff > kReportInterval) {
    SPDLOG_WARN("resource {}: mem usage {}, deallocate {}",
        mName, mCurMemUsage, bytes);
    mLastReportMemUsage = mCurMemUsage;
  }
  mUpStream->deallocate(ptr, bytes, alignment);
}
bool TrackingMemoryResource::do_is_equal(const std::pmr::memory_resource& other) const noexcept {
  return this == &other;
}

}  // namespace gringofts

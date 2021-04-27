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

#ifndef SRC_INFRA_UTIL_TRACKINGMEMORYRESOURCE_H_
#define SRC_INFRA_UTIL_TRACKINGMEMORYRESOURCE_H_

#include <memory_resource>
#include <string>

namespace gringofts {
class TrackingMemoryResource : public std::pmr::memory_resource {
 public:
  TrackingMemoryResource(
      const std::string &name,
      std::pmr::memory_resource* upstream = std::pmr::get_default_resource());

  void* do_allocate(size_t bytes, size_t alignment) override;
  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override;
  bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

 private:
  std::string mName;
  std::pmr::memory_resource* mUpStream;
  uint64_t mCurMemUsage = 0;
  uint64_t mLastReportMemUsage = 0;
  /// report memory usage every 100MB memory change
  static constexpr uint64_t kReportInterval = (uint64_t)100 * 1024 * 1024;
};
}  // namespace gringofts

#endif  // SRC_INFRA_UTIL_TRACKINGMEMORYRESOURCE_H_

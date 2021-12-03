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
#include <string>
#include <unistd.h>

#ifndef MAC_OS
#include <memory_resource>
#else
/// LLVM doesn't support PMR right now
/// using boost libraries
#include <boost/container/pmr/memory_resource.hpp>
#include <boost/container/pmr/global_resource.hpp>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>

#include <boost/container/pmr/map.hpp>
#include <boost/container/pmr/set.hpp>
namespace std::pmr {
using memory_resource = boost::container::pmr::memory_resource;
using monotonic_buffer_resource = boost::container::pmr::monotonic_buffer_resource;

template <class _CharT, class _Traits = char_traits<_CharT>>
using basic_string =
_VSTD::basic_string<_CharT, _Traits, boost::container::pmr::polymorphic_allocator<_CharT>>;
typedef basic_string<char> string;

template<class _Key, class _Value, class _Compare = less<_Key>>
using map = boost::container::pmr::map<_Key, _Value, _Compare>;

template<class _Key, class _Compare = less<_Key>>
using set = boost::container::pmr::set<_Key, _Compare>;

using boost::container::pmr::get_default_resource;
using boost::container::pmr::new_delete_resource;
using boost::container::pmr::null_memory_resource;
};  // namespace std::pmr
#endif

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

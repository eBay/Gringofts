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

#ifndef SRC_INFRA_UTIL_PERFCONFIG_H_
#define SRC_INFRA_UTIL_PERFCONFIG_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <rocksdb/iostats_context.h>
#include <rocksdb/perf_context.h>

namespace gringofts {

class PerfConfig {
 public:
  static PerfConfig &getInstance();
  void setProcessOutlierTime(uint64_t timeInMills);
  void setApplyOutlierTime(uint64_t timeInMills);
  void setReplyOutlierTime(uint64_t timeInMills);
  void setRocksDBPerfLevel(uint64_t perfLevel);
  void setGCEnabled(bool enabled);
  void setGCWorkTime(uint64_t timeInMills);
  void setGCIdleTime(uint64_t timeInMills);
  void setMaxGCDataSize(uint64_t maxGCDataSize);
  void setMaxMemoryPoolSizeInMB(uint64_t maxMemoryPoolSizeInMB);
  void setMemoryPoolType(const std::string &poolType);

  uint64_t getProcessOutlierTime() const;
  uint64_t getApplyOutlierTime() const;
  uint64_t getReplyOutlierTime() const;
  rocksdb::PerfLevel getRocksDBPerfLevel() const;
  bool isGCEnabled() const;
  uint64_t getGCWorkTime() const;
  uint64_t getGCIdleTime() const;
  uint64_t getMaxGCDataSize() const;
  uint64_t getMaxMemoryPoolSizeInMB() const;
  std::string getMemoryPoolType() const;

 private:
  PerfConfig();

  uint64_t mProcessOutlierTimeInMills = std::numeric_limits<uint64_t>::max();
  uint64_t mApplyOutlierTimeInMills = std::numeric_limits<uint64_t>::max();
  uint64_t mReplyOutlierTimeInMills = std::numeric_limits<uint64_t>::max();

  bool mGCEnabled = true;                   /// enable bg gc by default
  uint64_t mGCWorkTimeInMills = 2;          /// 2ms by default
  uint64_t mGCIdleTimeInMills = 5;          /// 5ms by default
  uint64_t mMaxGCDataSize = 1000 * 10000;   /// 1000KW items
  uint64_t mMaxMemoryPoolSizeInMB = 1024 * 2;  /// 2G memory pool reserved
  std::string mMemoryPoolType = "monotonic";
  rocksdb::PerfLevel mRocksdbPerfLevel = rocksdb::PerfLevel::kDisable;
};

}  // namespace gringofts

#endif  // SRC_INFRA_UTIL_PERFCONFIG_H_

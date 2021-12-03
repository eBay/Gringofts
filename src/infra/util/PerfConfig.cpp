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

#include "PerfConfig.h"

#include <spdlog/spdlog.h>

namespace gringofts {

PerfConfig &PerfConfig::getInstance() {
  static PerfConfig instance;
  return instance;
}

PerfConfig::PerfConfig() {
}

void PerfConfig::setProcessOutlierTime(uint64_t timeInMills) {
  mProcessOutlierTimeInMills = timeInMills;
  SPDLOG_INFO("setting process outlier time: {}", timeInMills);
}

void PerfConfig::setApplyOutlierTime(uint64_t timeInMills) {
  mApplyOutlierTimeInMills = timeInMills;
  SPDLOG_INFO("setting apply outlier time: {}", timeInMills);
}

void PerfConfig::setReplyOutlierTime(uint64_t timeInMills) {
  mReplyOutlierTimeInMills = timeInMills;
  SPDLOG_INFO("setting reply outlier time: {}", timeInMills);
}

void PerfConfig::setRocksDBPerfLevel(uint64_t perfLevel) {
  /// ref to https://github.com/facebook/rocksdb/wiki/Perf-Context-and-IO-Stats-Context
  switch (perfLevel) {
    case 0:
      mRocksdbPerfLevel = rocksdb::PerfLevel::kDisable;
      break;
    case 1:
      mRocksdbPerfLevel = rocksdb::PerfLevel::kEnableCount;
      break;
    case 2:
      mRocksdbPerfLevel = rocksdb::PerfLevel::kEnableTimeAndCPUTimeExceptForMutex;
      break;
    case 3:
      mRocksdbPerfLevel = rocksdb::PerfLevel::kEnableTimeExceptForMutex;
      break;
    case 4:
      mRocksdbPerfLevel = rocksdb::PerfLevel::kEnableTime;
      break;
    default:
      SPDLOG_ERROR("invalid perf level for rocksdb: {}", perfLevel);
  }
  SPDLOG_INFO("setting rocksdb perf level: {}", perfLevel);
}

void PerfConfig::setGCEnabled(bool enabled) {
  mGCEnabled = enabled;
  SPDLOG_INFO("setting gc enable flag: {}", enabled);
}

void PerfConfig::setGCWorkTime(uint64_t timeInMills) {
  mGCWorkTimeInMills = timeInMills;
  SPDLOG_INFO("setting gc work time: {}", timeInMills);
}

void PerfConfig::setGCIdleTime(uint64_t timeInMills) {
  mGCIdleTimeInMills = timeInMills;
  SPDLOG_INFO("setting gc idle time: {}", timeInMills);
}

void PerfConfig::setMaxGCDataSize(uint64_t maxGCDataSize) {
  mMaxGCDataSize = maxGCDataSize;
  SPDLOG_INFO("setting max gc data size: {}", maxGCDataSize);
}

void PerfConfig::setMaxMemoryPoolSizeInMB(uint64_t maxMemoryPoolSizeInMB) {
  mMaxMemoryPoolSizeInMB = maxMemoryPoolSizeInMB;
  SPDLOG_INFO("setting max memory pool size in MB: {}", maxMemoryPoolSizeInMB);
}

void PerfConfig::setMemoryPoolType(const std::string &poolType) {
  mMemoryPoolType = poolType;
  SPDLOG_INFO("setting memory pool type : {}", poolType);
}

uint64_t PerfConfig::getProcessOutlierTime() const {
  return mProcessOutlierTimeInMills;
}
uint64_t PerfConfig::getApplyOutlierTime() const {
  return mApplyOutlierTimeInMills;
}
uint64_t PerfConfig::getReplyOutlierTime() const {
  return mReplyOutlierTimeInMills;
}
rocksdb::PerfLevel PerfConfig::getRocksDBPerfLevel() const {
  return mRocksdbPerfLevel;
}

bool PerfConfig::isGCEnabled() const {
  return mGCEnabled;
}

uint64_t PerfConfig::getGCWorkTime() const {
  return mGCWorkTimeInMills;
}

uint64_t PerfConfig::getGCIdleTime() const {
  return mGCIdleTimeInMills;
}

uint64_t PerfConfig::getMaxGCDataSize() const {
  return mMaxGCDataSize;
}

uint64_t PerfConfig::getMaxMemoryPoolSizeInMB() const {
  return mMaxMemoryPoolSizeInMB;
}

std::string PerfConfig::getMemoryPoolType() const {
  return mMemoryPoolType;
}

}  /// namespace gringofts

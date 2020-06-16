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

#include "SyncPointProcessor.h"

#include <spdlog/spdlog.h>

namespace gringofts {

SyncPointProcessor& SyncPointProcessor::getInstance() {
  static SyncPointProcessor processor;
  return processor;
}

void SyncPointProcessor::setup(const std::vector<SyncPoint> &points) {
  std::lock_guard<std::mutex> lock(mMutex);
  mPoints.clear();
  for (const auto &p : points) {
    mPoints[p.mKey] = {p, false};
  }
  mCond.notify_all();
}

void SyncPointProcessor::tearDown() {
  std::unique_lock<std::mutex> lock(mMutex);
  while (mRunningCBCnt > 0) {
    SPDLOG_INFO("there are {} cb running", mRunningCBCnt);
    mCond.wait(lock);
  }
  mPoints.clear();
  SPDLOG_INFO("syncpoint teardown");
}

void SyncPointProcessor::reset(const std::vector<SyncPoint> &points) {
  disableProcessing();
  tearDown();
  setup(points);
  enableProcessing();
}

bool SyncPointProcessor::areAllPredecessorsCleared(const PointKey& point) {
  assert(mPoints.find(point) != mPoints.end());
  for (auto &pre : mPoints[point].first.mPredecessors) {
    if (!mPoints[pre].second) {
      /// one predecessor is not cleared
      return false;
    }
  }
  return true;
}

void SyncPointProcessor::Process(const PointKey &point, void *arg1, void *arg2) {
  if (!mEnabled) {
    return;
  }
  std::unique_lock<std::mutex> lock(mMutex);
  if (mPoints.find(point) == mPoints.end()) {
    /// ignore all unregistered points
    return;
  }

  while (!areAllPredecessorsCleared(point)) {
    if (mPoints[point].first.mType == SyncPointType::Ignore) {
      return;
    } else {
      mCond.wait(lock);
    }
  }

  auto &p = mPoints[point];
  mRunningCBCnt++;
  mMutex.unlock();
  p.first.mCB(arg1, arg2);
  mMutex.lock();
  mRunningCBCnt--;
  /// mark this point processed
  p.second = true;
  mCond.notify_all();
}

}  /// namespace gringofts

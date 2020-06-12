/**
 * Copyright (c) 2020 eBay Software Foundation. All rights reserved.
 */

#ifndef TEST_TEST_UTIL_SYNCPOINTPROCESSOR_H_
#define TEST_TEST_UTIL_SYNCPOINTPROCESSOR_H_

#include <assert.h>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "../../../src/infra/util/TestPointProcessor.h"

namespace gringofts {

using SyncPointCallBack = std::function<void(void*, void*)>;

enum SyncPointType {
  Ignore,     // if predecessor is not cleared, this point is disabled
  Block       // if predecessor is not cleared, this point will wait
};

struct SyncPoint {
  PointKey mKey;
  SyncPointCallBack mCB = nullptr;
  std::vector<PointKey> mPredecessors;
  SyncPointType mType = SyncPointType::Ignore;
};

class SyncPointProcessor : public TestPointProcessor {
 public:
    static SyncPointProcessor& getInstance();

    SyncPointProcessor(const SyncPointProcessor &) = delete;
    SyncPointProcessor &operator=(const SyncPointProcessor &) = delete;
    SyncPointProcessor(SyncPointProcessor &&) = delete;
    SyncPointProcessor &operator=(SyncPointProcessor &&) = delete;

    void setup(const std::vector<SyncPoint> &points);
    void tearDown();
    void reset(const std::vector<SyncPoint> &points);

    bool areAllPredecessorsCleared(const PointKey& point);

    void enableProcessing() {
      mEnabled = true;
    }
    void disableProcessing() {
      mEnabled = false;
    }
    void Process(const PointKey &point, void *arg1 = nullptr, void *arg2 = nullptr) override;

 private:
    SyncPointProcessor() = default;
    ~SyncPointProcessor() = default;

    /// pair<SyncPoint, bool>: point and its status to indicate whether it is triggered
    std::map<PointKey, std::pair<SyncPoint, bool>> mPoints;
    std::atomic<bool> mEnabled = false;

    /// guard against mPoints and mRunningCBCnt
    std::mutex mMutex;
    std::condition_variable mCond;
    uint32_t mRunningCBCnt = 0;
};

}  /// namespace gringofts

#endif  // TEST_TEST_UTIL_SYNCPOINTPROCESSOR_H_

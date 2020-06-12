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

#include <condition_variable>

#include <gtest/gtest.h>

#include "../../../src/infra/monitor/MonitorTypes.h"
#include "../../../src/infra/monitor/MonitorCenter.h"

namespace gringofts::test {

class TestMonitorable : public Monitorable {
 public:
  TestMonitorable() : mTermCallFlag(false), mRoleCallFlag(false) {
    MetricTag role_tag("current_role_v2", [this]() {
                         return getCurrentRole();
                       },
                       {{"leader", std::string("s1")},
                        {"candidate", std::string("s2")},
                        {"follower", std::string("s3")}});
    mTags.push_back(std::move(role_tag));
    // term
    mTags.emplace_back("current_term_v2", [this]() {
      return getCurrentTerm();
    });
  }
  std::atomic_bool mTermCallFlag;
  std::atomic_bool mRoleCallFlag;
  std::condition_variable cv;
  std::mutex mMutex;
  double getCurrentTerm() {
    mTermCallFlag = true;
    std::unique_lock lock(mMutex);
    cv.notify_all();
    return 99;
  }
  double getCurrentRole() {
    mRoleCallFlag = true;
    std::unique_lock lock(mMutex);
    cv.notify_all();
    return 1;
  }
  bool called() const {
    return mTermCallFlag && mRoleCallFlag;
  }
};

TEST(MonitorCenterTest, monitorTest) {
  auto test_monitorable = std::make_shared<TestMonitorable>();
  MonitorCenter monitor_center;
  monitor_center.registry(test_monitorable);
  {
    std::unique_lock lock(test_monitorable->mMutex);
    auto now = std::chrono::system_clock::now();
    test_monitorable->cv.wait_until(lock,
                                    now + std::chrono::milliseconds(5000),
                                    [&test_monitorable]() {
                                      return test_monitorable->called();
                                    });
  }
  EXPECT_EQ(test_monitorable->called(), true);
}

TEST(MonitorCenterTest, monitorTypesTest) {
  // couter shortcut
  auto couter1 = getCounter("counter1", {});
  couter1.increase();
  auto couter2 = getAppInfoMetricsCenter().counter("counter1", {});
  EXPECT_EQ(couter2.value(), couter2.value());
  // gauge shortcut
  auto guageShortCut = getGauge("gauge1", {});
  guageShortCut.set(99);
  auto guage = getAppInfoMetricsCenter().gauge("gauge1", {});
  EXPECT_EQ(guageShortCut.value(), guage.value());
}

}  /// namespace gringofts::test

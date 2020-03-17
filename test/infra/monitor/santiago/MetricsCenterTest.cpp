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

#include <map>

#include <gmock/gmock.h>

#include "../../../../src/infra/monitor/santiago/MetricsCenter.h"

class MetricsCenterTest : public ::testing::Test {};

decltype(auto) FirstMetrics(std::shared_ptr<prometheus::Registry> ptr) {
  return ptr->Collect()[0].metric[0];
}

TEST_F(MetricsCenterTest, CounterTest) {
  santiago::MetricsCenter metrics_center;
  auto name = "lalala";
  std::map<std::string, std::string> labels = {{"name", "hao"}};
  metrics_center.counter(name, labels).increase(10);
  auto registry = metrics_center.getRegistryPtr();
  EXPECT_EQ(10, FirstMetrics(metrics_center.getRegistryPtr()).counter.value);
  auto Counter = metrics_center.counter(name, labels);
  EXPECT_EQ(10, Counter.value());
  Counter.increase();
  EXPECT_EQ(11, FirstMetrics(registry).counter.value);
}

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

#include <thread>

#include <gmock/gmock.h>

#include "../../../../src/infra/monitor/santiago/metrics_collector.h"

using prometheus::Registry;
using santiago::MetricsCollector;
using testing::Test;

class MetricsCollectorTest : public Test {};

TEST_F(MetricsCollectorTest, registerMetric_success) {
  auto &collector = MetricsCollector::Instance();
  auto &family = prometheus::BuildCounter()
      .Name("sample_count")
      .Help("my sample counter")
      .Register(*collector.GetRegistry());
  auto &counter = family.Add({{"status", "success"}});
  collector.registerMetric("name", counter);
  EXPECT_EQ(&counter, collector.findMetric<prometheus::Counter>("name"));
}


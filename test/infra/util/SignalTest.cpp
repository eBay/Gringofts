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

#include <gtest/gtest.h>
#include <iostream>

#include "../../../src/infra/util/Signal.h"

namespace gringofts::test {

class TestSignal1 : public Signal {};
class TestSignal2 : public Signal {};

TEST(SignalUtilTest, SignalTest) {
  SignalSlot ss;
  int cnt = 0;
  std::mutex mutex;
  std::condition_variable cv;
  ss.handle<TestSignal1>([&mutex, &cv, &cnt](const Signal &) {
    std::unique_lock lk{mutex};
    std::cout << "signal 1 triggered" << std::endl;
    cnt++;
    cv.notify_one();
  });
  ss.handle<TestSignal2>([&mutex, &cv, &cnt](const Signal &) {
    std::unique_lock lk{mutex};
    std::cout << "signal 2 triggered" << std::endl;
    cnt++;
    cv.notify_one();
  });
  ss << std::make_shared<TestSignal1>();
  ss << std::make_shared<TestSignal2>();
  {
    std::unique_lock lk{mutex};
    cv.wait(lk, [&cnt]() { return cnt == 2; });
  }
}

class TestSignal3 : public FutureSignal<int> {};

TEST(SignalUtilTest, SignalWithFuture) {
  SignalSlot ss;
  std::atomic_int cnt = 0;
  std::atomic_bool running = true;
  ss.handle<TestSignal3>([&cnt](const Signal &s) {
    const auto &s3 = dynamic_cast<const TestSignal3 &>(s);
    s3.passValue(cnt);
  });
  std::thread t1([&cnt, &running]() {
    while (running) {
      cnt++;
    }
  });
  auto signal = std::make_shared<TestSignal3>();
  ss << signal;
  int val = signal->getFuture().get();
  std::cout << val << std::endl;
  running = false;
  t1.join();
}
}  // namespace gringofts::test

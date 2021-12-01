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

#ifndef SRC_INFRA_UTIL_TIMEUTIL_H_
#define SRC_INFRA_UTIL_TIMEUTIL_H_

#include <chrono>
#include <stdint.h>

namespace gringofts {

using TimestampInNanos = uint64_t;
using TimestampInMills = uint64_t;
using ElapsedTimeInNanos = uint64_t;

class TimeUtil final {
 public:
  /**
   * Return the time since the Unix epoch in nanoseconds.
   */
  static TimestampInNanos currentTimeInNanos() {
    struct timespec now;
    int r = clock_gettime(CLOCK_REALTIME, &now);
    assert(r == 0);
    return uint64_t(now.tv_sec) * 1000 * 1000 * 1000 + uint64_t(now.tv_nsec);
  }

  /**
   * Convert timestamp from nanosecond to millisecond
   */
  static uint64_t nanosToMillis(uint64_t nanos) {
    return nanos / 1000000;
  }

  /**
   * Calculate elapse time in millisecond
   */
  static uint64_t elapseTimeInMillis(TimestampInNanos from, TimestampInNanos to) {
    return from < to ? nanosToMillis(to - from) : 0;
  }

  static std::string_view currentUTCTime() {
    using ::std::chrono::system_clock;
    thread_local static char buf[32] = {0};
    struct tm utc_tm;
    system_clock::time_point now = system_clock::now();
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    time_t tt = system_clock::to_time_t(now);
    gmtime_r(&tt, &utc_tm);
    snprintf(buf, sizeof(buf), "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3dZ", utc_tm.tm_year + 1900,
             utc_tm.tm_mon + 1, utc_tm.tm_mday, utc_tm.tm_hour, utc_tm.tm_min,
             utc_tm.tm_sec, static_cast<unsigned>(ms % 1000));
    return std::string_view(buf);
  }
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_TIMEUTIL_H_

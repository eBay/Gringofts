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

#ifndef SRC_INFRA_UTIL_TESTPOINTPROCESSOR_H_
#define SRC_INFRA_UTIL_TESTPOINTPROCESSOR_H_

#include <string>

#define TEST_POINT(testPointPtr, pointKey) \
  if (testPointPtr != nullptr) { \
    testPointPtr->Process(pointKey); \
  }

#define TEST_POINT_WITH_TWO_ARGS(testPointPtr, pointKey, arg1, arg2) \
  if (testPointPtr != nullptr) { \
    testPointPtr->Process(pointKey, arg1, arg2); \
  }

namespace gringofts {

enum TPRegistry {
  /// format: class_method_purpose
  RaftCore_receiveMessage_interceptIncoming,
  RaftCore_electionTimeout_interceptTimeout
};

using PointKey = TPRegistry;

class TestPointProcessor {
 public:
    virtual void Process(const PointKey &point, void *arg1 = nullptr, void *arg2 = nullptr) = 0;
};

}  // namespace gringofts

#endif  // SRC_INFRA_UTIL_TESTPOINTPROCESSOR_H_

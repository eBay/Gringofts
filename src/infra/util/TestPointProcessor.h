/**
 * Copyright (c) 2020 eBay Software Foundation. All rights reserved.
 */
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

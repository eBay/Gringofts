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

#ifndef SRC_INFRA_GRPC_REQUESTHANDLE_H_
#define SRC_INFRA_GRPC_REQUESTHANDLE_H_

#include <optional>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include "../../infra/util/TimeUtil.h"
#include "../monitor/MonitorTypes.h"

namespace gringofts {

/**
 * A tag interface to uniquely identify the grpc request.
 * This is used for asynchronous/non-blocking grpc, see https://grpc.io/docs/tutorials/async/helloasync-cpp.html.
 */
class RequestHandle {
 public:
  RequestHandle() = default;

  virtual ~RequestHandle() {
    auto summary = getSummary("request_call_latency_in_ms", {});
    summary.observe((TimeUtil::currentTimeInNanos() - mCommandCreateTime) / 1000000.0);
  }

  /**
   * Wrap the request handling logic
   */
  virtual void proceed() = 0;

  /**
 * Method called when ok is false after calling Next against the CompletionQueue
 */
  virtual void failOver() = 0;

  /**
   * Async callback
   */
  virtual void fillResultAndReply(uint32_t code,
                                  const std::string &message,
                                  std::optional<uint64_t> leaderId) = 0;

  /**
   * Validate if this is a valid test
   * Return true if it is valid, otherwise, return false
   */
  bool validateRequest(const std::multimap<grpc::string_ref, grpc::string_ref> &metadata, bool testModeEnabled) {
    auto isTest = false;
    auto iter = metadata.find("x-request-type");
    if (iter != metadata.end()) {
      std::string value((iter->second).data(), (iter->second).length());
      isTest = value == "test";
    }

    if (isTest && !testModeEnabled) {
      SPDLOG_WARN("Reject test request as this it only takes real traffic.");
      fillResultAndReply(503, "Test request sent to non-test PU", std::nullopt);
      return false;
    }

    if (!isTest && testModeEnabled) {
      SPDLOG_WARN("Reject real request as this it only takes test traffic.");
      fillResultAndReply(503, "Real request sent to test PU", std::nullopt);
      return false;
    }

    return true;
  }

 protected:
  // command create time in nanos
  TimestampInNanos mCommandCreateTime;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_GRPC_REQUESTHANDLE_H_

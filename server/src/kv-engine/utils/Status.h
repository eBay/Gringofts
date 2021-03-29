/************************************************************************
Copyright 2021-2022 eBay Inc.
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

#ifndef SERVER_SRC_KV_ENGINE_UTILS_STATUS_H_
#define SERVER_SRC_KV_ENGINE_UTILS_STATUS_H_

#include <assert.h>

namespace goblin::kvengine::utils {

class Status {
 public:
  enum Code {
     OK = 0,
     ERROR = 1,
     OOM = 2,
     NOTFOUND = 3,
     PRECOND_UNMATCHED = 4,
     NOTSUPPORTED = 5,
     INVALID_ARG = 6,
     EXPIRED = 7,
     NOT_LEADER = 8,
     STOPPED = 9,
     CLIENT_ROUTE_OUT_OF_DATE = 10,
     SERVER_ROUTE_OUT_OF_DATE = 11,
     OUT_OF_SERVICE = 12,
     WRONG_ROUTE = 13,      /// the key is routed to a wrong node
     MIGRATED_ROUTE = 14     /// the key should be routed to another node
  };

  explicit Status(const Code &code, const std::string &msg = ""): mCode(code), mMsg(msg) {}

  bool isOK() const {
     return mCode == Code::OK;
  }
  bool isOOM() const {
     return mCode == Code::OOM;
  }
  bool isNotFound() const {
     return mCode == Code::NOTFOUND;
  }
  bool isPrecondUnMatched() const {
     return mCode == Code::PRECOND_UNMATCHED;
  }
  bool isNotSupported() const {
     return mCode == Code::NOTSUPPORTED;
  }
  bool isInvalidArg() const {
     return mCode == Code::INVALID_ARG;
  }
  bool isExpired() const {
     return mCode == Code::EXPIRED;
  }
  bool isNotLeader() const {
     return mCode == Code::NOT_LEADER;
  }
  bool isStopped() const {
     return mCode == Code::STOPPED;
  }
  bool isClientRouteOutOfDate() const {
     return mCode == Code::CLIENT_ROUTE_OUT_OF_DATE;
  }
  bool isServerRouteOutOfDate() const {
     return mCode == Code::SERVER_ROUTE_OUT_OF_DATE;
  }
  bool isOutOfService() const {
     return mCode == Code::OUT_OF_SERVICE;
  }
  bool isWrongRoute() const {
     return mCode == Code::WRONG_ROUTE;
  }
  bool isMigratedRoute() const {
     return mCode == Code::MIGRATED_ROUTE;
  }
  Code getCode() const {
     return mCode;
  }
  std::string getDetail() const {
     return mMsg;
  }
  static Status ok() {
     return Status(Code::OK);
  }
  static Status error(const std::string &msg = "") {
     return Status(Code::ERROR, msg);
  }
  static Status oom(const std::string &msg = "") {
     return Status(Code::OOM, msg);
  }
  static Status notFound(const std::string &msg = "") {
     return Status(Code::NOTFOUND, msg);
  }
  static Status precondUnmatched(const std::string &msg = "") {
     return Status(Code::PRECOND_UNMATCHED, msg);
  }
  static Status notSupported(const std::string &msg = "") {
     return Status(Code::NOTSUPPORTED, msg);
  }
  static Status invalidArg(const std::string &msg = "") {
     return Status(Code::INVALID_ARG, msg);
  }
  static Status expired(const std::string &msg = "") {
    return Status(Code::EXPIRED, msg);
  }
  static Status notLeader(const std::string &msg = "") {
    return Status(Code::NOT_LEADER, msg);
  }
  static Status stopped(const std::string &msg = "") {
    return Status(Code::STOPPED, msg);
  }
  static Status clientRouteOutOfDate(const std::string &msg = "") {
    return Status(Code::CLIENT_ROUTE_OUT_OF_DATE, msg);
  }
  static Status serverRouteOutOfDate(const std::string &msg = "") {
    return Status(Code::SERVER_ROUTE_OUT_OF_DATE, msg);
  }
  static Status outOfService(const std::string &msg = "") {
    return Status(Code::OUT_OF_SERVICE, msg);
  }
  static Status wrongRoute(const std::string &msg = "") {
    return Status(Code::WRONG_ROUTE, msg);
  }
  static Status migratedRoute(const std::string &msg = "") {
    return Status(Code::MIGRATED_ROUTE, msg);
  }
  static void assertOK(const Status &s) {
     assert(s.isOK());
  }

 private:
  Code mCode;
  std::string mMsg;
};

}  /// namespace goblin::kvengine::utils

#endif  // SERVER_SRC_KV_ENGINE_UTILS_STATUS_H_


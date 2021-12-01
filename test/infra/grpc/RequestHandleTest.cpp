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
#include <gtest/gtest.h>

#include "../../../src/infra/grpc/RequestHandle.h"

namespace gringofts::test {

class RequestHandleMock : public RequestHandle {
 public:
  MOCK_METHOD0(proceed, void());
  MOCK_METHOD0(failOver, void());
  MOCK_METHOD3(fillResultAndReply, void(
      uint32_t,
      const std::string&,
      std::optional<uint64_t>));
};

TEST(RequestHandleTest, ValidateTestTag) {
  /// init
  testing::NiceMock<RequestHandleMock> mock;
  std::multimap<grpc::string_ref, grpc::string_ref> metadata{{"x-request-type", "test"}};

  /// assert
  EXPECT_FALSE(mock.validateRequest(metadata, false));
  EXPECT_TRUE(mock.validateRequest(metadata, true));
}

TEST(RequestHandleTest, ValidateNonTestTag) {
  /// init
  testing::NiceMock<RequestHandleMock> mock;
  std::multimap<grpc::string_ref, grpc::string_ref> metadata{{"x-request-type", "non-test"}};

  /// assert
  EXPECT_FALSE(mock.validateRequest(metadata, true));
  EXPECT_TRUE(mock.validateRequest(metadata, false));
}

}  /// namespace gringofts::test

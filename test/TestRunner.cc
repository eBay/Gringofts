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

#include <google/protobuf/stubs/common.h>
#include <gtest/gtest.h>

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::UnitTest &unitTest = *::testing::UnitTest::GetInstance();
  ::testing::TestEventListeners &listeners = unitTest.listeners();
  // add listeners here
  int ret = RUN_ALL_TESTS();
  google::protobuf::ShutdownProtobufLibrary();
  return ret;
}

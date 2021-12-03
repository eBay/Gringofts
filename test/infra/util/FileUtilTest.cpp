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

#include "../../../src/infra/util/FileUtil.h"
#include "../../../src/infra/util/Util.h"

namespace gringofts::test {

TEST(FileUtilTest, SetFileContentTest) {
  /// init
  std::string fileName = testing::TempDir() + "/tests";
  FileUtil::deleteFile(fileName);
  EXPECT_FALSE(FileUtil::fileExists(fileName));

  /// behavior
  FileUtil::setFileContent(fileName, "aaaa");

  /// assert
  EXPECT_TRUE(FileUtil::fileExists(fileName));
  EXPECT_EQ(FileUtil::getFileContent(fileName), std::string("aaaa"));

  /// cleanup
  FileUtil::deleteFile(fileName);
  EXPECT_FALSE(FileUtil::fileExists(fileName));
}

TEST(FileUtilTest, SetFileContentWithSyncTest) {
  /// init
  std::string fileName = testing::TempDir() + "/tests";
  FileUtil::deleteFile(fileName);
  EXPECT_FALSE(FileUtil::fileExists(fileName));

  /// behavior
  FileUtil::setFileContentWithSync(fileName, "bbbb");

  /// assert
  EXPECT_TRUE(FileUtil::fileExists(fileName));
  EXPECT_EQ(FileUtil::getFileContent(fileName), std::string("bbbb"));

  /// cleanup
  FileUtil::deleteFile(fileName);
  EXPECT_FALSE(FileUtil::fileExists(fileName));
}

TEST(FileUtilTest, ListFilesTest) {
  /// init
  const auto &tempDir = testing::TempDir() + "/tests/";
  Util::executeCmd("mkdir -p " + tempDir + "tests2");
  Util::executeCmd("touch " + tempDir + "1");
  Util::executeCmd("touch " + tempDir + "2");
  Util::executeCmd("touch " + tempDir + "tests2/3");

  /// behavior
  auto files = FileUtil::listFiles(tempDir);
  std::sort(files.begin(), files.end());
  std::vector<std::string> expected{tempDir + "1", tempDir + "2"};

  /// assert
  EXPECT_EQ(files, expected);

  /// cleanup
  Util::executeCmd("rm -rf " + tempDir);
}

TEST(FileUtilTest, FileSizeTest) {
  /// init
  const auto &fileName = testing::TempDir() + "/testFile";
  FileUtil::deleteFile(fileName);
  auto fd = ::open(fileName.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_EXCL, 0644);

  /// behavior
  auto expected = 4096 * 6;
  FileUtil::setFileSize(fd, expected);
  auto size = FileUtil::getFileSize(fd);

  /// assert
  EXPECT_EQ(size, expected);

  /// cleanup
  ::close(fd);
  FileUtil::deleteFile(fileName);
}

/// TODO: Re-enable this test
TEST(FileUtilTest, DISABLED_ErrorScenariosTest) {
  /// init
  std::ifstream ifs(testing::TempDir() + "/ifs");
  ifs.setstate(ifs.badbit);

  std::ofstream ofs(testing::TempDir() + "/ofs");
  ofs.setstate(ofs.badbit);

  /// assert
  EXPECT_EXIT(FileUtil::checkFileState(ifs), ::testing::ExitedWithCode(255), "");
  EXPECT_EXIT(FileUtil::checkFileState(ofs), ::testing::ExitedWithCode(255), "");
}

}  /// namespace gringofts::test

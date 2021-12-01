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

#ifndef SRC_INFRA_UTIL_FILEUTIL_H_
#define SRC_INFRA_UTIL_FILEUTIL_H_

#include <fstream>
#include <vector>
#include <string>

namespace gringofts {

class FileUtil final {
 public:
  /**
   * Return true if the file exists, otherwise return false.
   */
  static bool fileExists(const std::string &fileName);

  /**
   * Return true if the file deleted, otherwise return false.
   */
  static bool deleteFile(const std::string &fileName);

  /**
   * Return the current working dir
   */
  static std::string currentWorkingDir();

  /**
   * Get content of specified file
   */
  static std::string getFileContent(const std::string &fileName);

  /**
   * Set content of specified file
   */
  static void setFileContent(const std::string &fileName, const std::string &content);

  /**
   * Set content of specified file, explicitly call fsync()
   *
   * ATTENTION: There is no (standard) way to extract the fd
   *            from std::fstream, since the standard library
   *            does not mandate how file streams will be implemented
   */
  static void setFileContentWithSync(const std::string &fileName, const std::string &content);

  /**
   * List names of regular file under specified dir.
   */
  static std::vector<std::string> listFiles(const std::string &dir_);

  /**
   * List names of directories under specified dir.
   */
  static std::vector<std::string> listDirs(const std::string &dir_);

  /**
   * given fd, get file size
   */
  static int64_t getFileSize(int fd);

  /**
   * given fd, set file size
   * fileSize should align to page size
   */
  static void setFileSize(int fd, uint64_t fileSize);

  /**
   * Sync a portion of mmap memory to disk.
   */
  static void syncAt(void *rawPtr, uint64_t length);

  /**
   * Write string to a binary file
   * Format: length of string (without nullterm) + string content (can contain nullterm)
   * Example: 6h ello
   * @param ofs file descriptor of a binary file
   * @param content the string content to be written
   */
  static void writeStrToFile(std::ofstream &ofs, const std::string &content);

  /**
   * Read string from a binary file
   * Format: length of string (without nullterm) + string content (can contain nullterm)
   * Example: 6h ello
   * @param ifs file descriptor of a binary file
   * @returns string
   */
  static std::string readStrFromFile(std::ifstream &ifs);

  /**
   * Write uint64 to a binary file
   * @param ofs file descriptor of a binary file
   * @param content value of uint64
   */
  static void writeUint64ToFile(std::ofstream &ofs, uint64_t content);

  /**
   * Read uint64 from a binary file
   * @param ifs file descriptor of a binary file
   * @returns value of uint64
   */
  static uint64_t readUint64FromFile(std::ifstream &ifs);

  /**
   * Sync file to disk
   * @param fileName name of the file
   */
  static void syncFile(const std::string &fileName);

  static void checkFileState(std::ifstream &ifs);

  static void checkFileState(std::ofstream &ofs);
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_FILEUTIL_H_

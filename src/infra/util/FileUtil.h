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

#include <assert.h>
#include <fcntl.h>
#include <fstream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <absl/strings/str_cat.h>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#include <spdlog/spdlog.h>

#include "Util.h"

namespace gringofts {

class FileUtil final {
 public:
  /**
   * Return true if the file exists, otherwise return false.
   */
  static bool fileExists(const std::string &fileName) {
    struct stat buffer;
    return (stat(fileName.c_str(), &buffer) == 0);
  }

  /**
   * Return true if the file deleted, otherwise return false.
   */
  static bool deleteFile(const std::string &fileName) {
    if (std::remove(fileName.c_str()) != 0) {
      SPDLOG_INFO("Delete {} failed, file may not exist.", fileName);
      return false;
    }
    // Delete file successfully.
    return true;
  }

  /**
   * Return the current working dir
   */
  static std::string currentWorkingDir() {
    char buff[FILENAME_MAX];
    assert(getcwd(buff, FILENAME_MAX));
    std::string currWorkingDir(buff);

    return currWorkingDir;
  }

  /**
   * Get content of specified file
   */
  static std::string getFileContent(const std::string &fileName) {
    std::ifstream fd(fileName);
    if (!fd) {
      throw std::runtime_error("failed to open file " + fileName);
    }
    return std::string(std::istreambuf_iterator<char>(fd), std::istreambuf_iterator<char>());
  }

  /**
   * Set content of specified file
   */
  static void setFileContent(const std::string &fileName,
                             const std::string &content) {
    std::ofstream fd(fileName);
    if (!fd) {
      throw std::runtime_error("failed to open file " + fileName);
    }
    fd << content;
  }

  /**
   * Set content of specified file, explicitly call fsync()
   *
   * ATTENTION: There is no (standard) way to extract the fd
   *            from std::fstream, since the standard library
   *            does not mandate how file streams will be implemented
   */
  static void setFileContentWithSync(const std::string &fileName,
                                     const std::string &content) {
    namespace fs = boost::filesystem;
    namespace io = boost::iostreams;

    const auto &tmpFileName = absl::StrCat(fileName, ".tmp");
    fs::path filePath(tmpFileName);
    io::stream<io::file_descriptor_sink> file(filePath);

    file << content;
    file.flush();

    /// sync to disk
#ifdef MAC_OS
    assert(0 == ::fsync(file->handle()));
#else
    assert(0 == ::fdatasync(file->handle()));
#endif
    assert(0 == ::rename(tmpFileName.c_str(), fileName.c_str()));
  }

  /**
   * List names of regular file under specified dir.
   */
  static std::vector<std::string> listFiles(const std::string &dir_) {
    namespace fs = boost::filesystem;

    fs::path dir(dir_);
    if (!fs::is_directory(dir)) {
      SPDLOG_WARN("dir '{}' is not a directory.", dir_);
      return {};
    }

    std::vector<std::string> buffer;

    fs::directory_iterator iter(dir);
    fs::directory_iterator end;

    while (iter != end) {
      if (fs::is_regular_file(iter->path())) {
        std::string fileName = iter->path().string();
        buffer.push_back(std::move(fileName));
      }
      ++iter;
    }

    return buffer;
  }

  /**
   * List names of directories under specified dir.
   */
  static std::vector<std::string> listDirs(const std::string &dir_) {
    namespace fs = boost::filesystem;

    fs::path dir(dir_);
    if (!fs::is_directory(dir)) {
      SPDLOG_WARN("dir '{}' is not a directory.", dir_);
      return {};
    }

    std::vector<std::string> buffer;

    fs::directory_iterator iter(dir);
    fs::directory_iterator end;

    while (iter != end) {
      if (fs::is_directory(iter->path())) {
        std::string dirName = iter->path().string();
        buffer.push_back(std::move(dirName));
      }
      ++iter;
    }

    return buffer;
  }

  /**
   * given fd, get file size
   */
  static int64_t getFileSize(int fd) {
    FILE *file = ::fdopen(fd, "rw");
    assert(file != nullptr);

    int ret = ::fseek(file, 0, SEEK_END);
    assert(ret == 0);

    int64_t fileSize = ::ftell(file);
    assert(fileSize != -1);
    return fileSize;
  }

  /**
   * given fd, set file size
   * fileSize should align to page size
   */
  static void setFileSize(int fd, uint64_t fileSize) {
    /// fileSize should align to page size, which is 4096 byte
    assert(fileSize % sysconf(_SC_PAGESIZE) == 0);

    off_t offset = ::lseek(fd, fileSize - 1, SEEK_SET);
    assert(offset == fileSize - 1);

    /**
     * Something needs to be written at the end of the file to
     * have the file actually have the new size.
     * Just writing an empty string at the current file position will do.
     *
     * Note:
     *  - The current position in the file is at the end of the stretched
     *    file due to the call to lseek().
     *  - An empty string is actually a single '\0' character, so a zero-byte
     *    will be written at the last byte of the file.
     */
    int ret = ::write(fd, "", 1);
    assert(ret != -1);
  }

  /**
   * Sync a portion of mmap memory to disk.
   */
  static void syncAt(void *rawPtr, uint64_t length) {
    uint64_t delta = reinterpret_cast<uint64_t>(rawPtr) % sysconf(_SC_PAGESIZE);

    /// rawPtr - offset should be a multiple of PAGESIZE
    int ret = ::msync(reinterpret_cast<uint8_t *>(rawPtr) - delta,
                      length + delta,
                      MS_SYNC);
    assert(ret == 0);
  }

  /**
   * Write string to a binary file
   * Format: length of string (without nullterm) + string content (can contain nullterm)
   * Example: 6h ello
   * @param ofs file descriptor of a binary file
   * @param content the string content to be written
   */
  static void writeStrToFile(std::ofstream &ofs, const std::string &content) {
    writeUint64ToFile(ofs, content.length());
    ofs.write(content.c_str(), content.length());
    /// TODO: Enhance file read/write error handling
    checkFileState(ofs);
  }

  /**
   * Read string from a binary file
   * Format: length of string (without nullterm) + string content (can contain nullterm)
   * Example: 6h ello
   * @param ifs file descriptor of a binary file
   * @returns string
   */
  static std::string readStrFromFile(std::ifstream &ifs) {
    const auto size = readUint64FromFile(ifs);
    assert(size > 0);
    std::vector<char> buffer;
    buffer.resize(size + 1);
    ifs.read(buffer.data(), size);
    /// TODO: Enhance file read/write error handling
    checkFileState(ifs);
    buffer[size] = '\0';
    return std::string(buffer.data());
  }

  /**
   * Write uint64 to a binary file
   * @param ofs file descriptor of a binary file
   * @param content value of uint64
   */
  static void writeUint64ToFile(std::ofstream &ofs, uint64_t content) {
    auto size = content;
    ofs.write(reinterpret_cast<const char *>(&size), sizeof(size));
    /// TODO: Enhance file read/write error handling
    checkFileState(ofs);
  }

  /**
   * Read uint64 from a binary file
   * @param ifs file descriptor of a binary file
   * @returns value of uint64
   */
  static uint64_t readUint64FromFile(std::ifstream &ifs) {
    uint64_t size;
    ifs.read(reinterpret_cast<char *>(&size), sizeof(size));
    /// TODO: Enhance file read/write error handling
    checkFileState(ifs);

    return size;
  }

  /**
   * Sync file to disk
   * @param fileName name of the file
   */
  static void syncFile(const std::string &fileName) {
#ifdef MAC_OS
    int fd = ::open(fileName.c_str(), O_RDONLY);
    assert(fd != -1);

    /// sync to disk
    ::fsync(fd);

    /// do not forget to close the fd
    ::close(fd);
#else
    int fd = ::open64(fileName.c_str(), O_RDONLY);
    assert(fd != -1);

    /// sync to disk
    ::fdatasync(fd);

    /// do not forget to close the fd
    ::close(fd);
#endif
  }

  static void checkFileState(std::ifstream &ifs) {
    if (!ifs) {
      SPDLOG_WARN("Read failed, bad={}, fail={}, eof={}", ifs.bad(), ifs.fail(), ifs.eof());
      exit(-1);
    }
  }

  static void checkFileState(std::ofstream &ofs) {
    if (!ofs) {
      SPDLOG_WARN("Write failed, bad={}, fail={}, eof={}", ofs.bad(), ofs.fail(), ofs.eof());
      exit(-1);
    }
  }
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_FILEUTIL_H_

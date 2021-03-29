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
#ifndef SERVER_SRC_KV_ENGINE_UTILS_STRUTIL_H_
#define SERVER_SRC_KV_ENGINE_UTILS_STRUTIL_H_

#include <regex>
#include <string>
#include <vector>

namespace goblin::kvengine::utils {

class StrUtil final {
 public:
  /**
   * Response a vector with splited parts
   * TODO: deco below method with abseil
   */
  static std::vector<std::string> tokenize(const std::string &str, char delim) {
    std::vector<std::string> buffer;

    std::size_t start = 0;
    std::size_t end = 0;

    while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
      end = str.find(delim, start);
      std::string temp = str.substr(start, end - start);
      std::string fixed = std::regex_replace(temp, std::regex("\n"), "");
      buffer.push_back(std::move(fixed));
    }

    return buffer;
  }

  /**
   * check string has suffix or not
   */
  static bool endsWith(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
  }

  /**
   * convert bytes started at data with length of len to
   * its hex format, for readability.
   */
  static std::string hexStr(const std::string &str) {
    return hexStr(reinterpret_cast<const unsigned char *>(str.c_str()), str.size());
  }

  static std::string hexStr(const unsigned char *data, uint64_t len) {
    static constexpr char characters[] = "0123456789abcdef";

    std::string hex;

    for (uint64_t i = 0; i < len; ++i) {
      hex += characters[data[i] >> 4];
      hex += characters[data[i] & 0x0F];
    }

    return hex;
  }

  static uint64_t simpleHash(const char *str) {
    uint64_t hash = 5381;
    int c;

    while (c = *str++)
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
  }

  template<typename ... Args>
  static std::string format(const std::string& format, Args ... args){
    size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1;  // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[ size ]);
    snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string( buf.get(), buf.get() + size - 1);  // We don't want the '\0' inside
  }
};

}  /// namespace goblin::kvengine::utils

#endif  // SERVER_SRC_KV_ENGINE_UTILS_STRUTIL_H_

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

#ifndef SRC_INFRA_UTIL_STRUTIL_H_
#define SRC_INFRA_UTIL_STRUTIL_H_

#include <regex>
#include <string>
#include <vector>

namespace gringofts {

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
   * Compliance with Golang multi-lines
   * [multilines]
   * two_lines = how about \
   *            continuation lines?
   * https://github.com/go-ini/ini/blob/master/testdata/full.ini
   */
  static std::vector<std::string> tokenizeGoMultiLines(const std::string &str, char delim) {
    std::string temp = std::regex_replace(str, std::regex("\\\\+"), "");
    return tokenize(std::ref(temp), delim);
  }

  static void replace(std::string *str, const std::string &old, const std::string &rep) {
    // Find and replace
    size_t pos = str->find(old);
    if (pos != std::string::npos) {
        str->replace(pos, old.length(), rep);
    }
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
};

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_STRUTIL_H_

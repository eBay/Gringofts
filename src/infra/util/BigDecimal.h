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

#ifndef SRC_INFRA_UTIL_BIGDECIMAL_H_
#define SRC_INFRA_UTIL_BIGDECIMAL_H_

#include <boost/multiprecision/cpp_dec_float.hpp>

namespace gringofts {

class BigDecimal {
 public:
  explicit BigDecimal(const std::string &number);
  explicit BigDecimal(uint64_t number);
  explicit BigDecimal(boost::multiprecision::cpp_dec_float_100 number);

  std::string toString() const;

  boost::multiprecision::cpp_dec_float_100 getValue() const {
    return mValue;
  }

  static bool isValid(const std::string &number);

 private:
  boost::multiprecision::cpp_dec_float_100 mValue;
};

inline BigDecimal operator+(const BigDecimal &a, const BigDecimal &b) {
  return BigDecimal(a.getValue() + b.getValue());
}

inline BigDecimal operator-(const BigDecimal &a, const BigDecimal &b) {
  return BigDecimal(a.getValue() - b.getValue());
}

inline BigDecimal operator*(const BigDecimal &a, const BigDecimal &b) {
  return BigDecimal(a.getValue() * b.getValue());
}

inline BigDecimal operator/(const BigDecimal &a, const BigDecimal &b) {
  return BigDecimal(a.getValue() / b.getValue());
}

inline BigDecimal operator-(const BigDecimal &a) {
  return BigDecimal(-a.getValue());
}

inline bool operator<(const BigDecimal &a, const BigDecimal &b) {
  return a.getValue() < b.getValue();
}

inline bool operator>(const BigDecimal &a, const BigDecimal &b) {
  return a.getValue() > b.getValue();
}

inline bool operator==(const BigDecimal &a, const BigDecimal &b) {
  return a.getValue() == b.getValue();
}

inline bool operator!=(const BigDecimal &a, const BigDecimal &b) {
  return a.getValue() != b.getValue();
}

inline bool operator<=(const BigDecimal &a, const BigDecimal &b) {
  return a.getValue() <= b.getValue();
}

inline bool operator>=(const BigDecimal &a, const BigDecimal &b) {
  return a.getValue() >= b.getValue();
}

}  /// namespace gringofts

#endif  // SRC_INFRA_UTIL_BIGDECIMAL_H_

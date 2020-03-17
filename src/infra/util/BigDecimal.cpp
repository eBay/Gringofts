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

#include "BigDecimal.h"

namespace gringofts {

bool BigDecimal::isValid(const std::string &number) {
  try {
    auto value = boost::multiprecision::cpp_dec_float_100(number);
    return true;
  }
  catch (std::runtime_error) {
    return false;
  }
}

BigDecimal::BigDecimal(const std::string &number) : mValue(number) {}

BigDecimal::BigDecimal(uint64_t number) : mValue(number) {}

BigDecimal::BigDecimal(boost::multiprecision::cpp_dec_float_100 number) : mValue(std::move(number)) {}

std::string BigDecimal::toString() const {
  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) << mValue;
  return stream.str();
}

}  /// namespace gringofts

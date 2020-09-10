/*
 * Copyright (c) 2020 eBay Software Foundation. All rights reserved.
 */
#ifndef SRC_INFRA_UTIL_PODUTIL_H_
#define SRC_INFRA_UTIL_PODUTIL_H_
#include "string"

namespace gringofts {

template<typename PODCLASS>
struct PodUtil {
  static PODCLASS Deserialize(const std::string &fromValue) {
    const PODCLASS *ptr = reinterpret_cast<const PODCLASS *>(fromValue.c_str());
    return PODCLASS{*ptr};
  }
  static std::string Serialize(const PODCLASS &value) {
    const char *data = reinterpret_cast<const char *>(&value);
    return std::string(data, sizeof(value));
  }
};
}  // namespace gringofts
#endif  // SRC_INFRA_UTIL_PODUTIL_H_

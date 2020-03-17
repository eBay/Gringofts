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

#include "AppStateMachine.h"

#include "../../infra/util/FileUtil.h"

namespace gringofts {
namespace demo {
namespace v1 {

bool AppStateMachine::createSnapshotAndPersist(uint64_t offset,
                                               std::ofstream &ofs,
                                               Crypto &crypto) const {
  FileUtil::writeStrToFile(ofs, "v1");

  crypto.beginEncryption(25 * (1 << 20));  /// 25MB

  crypto.encryptUint64ToFile(ofs, offset);
  crypto.encryptUint64ToFile(ofs, TimeUtil::currentTimeInNanos());
  crypto.encryptUint64ToFile(ofs, mValue);

  crypto.commitEncryption(ofs);

  return true;
}

std::optional<uint64_t> AppStateMachine::loadSnapshotFrom(std::ifstream &ifs,
                                                          const CommandDecoder &commandDecoder,
                                                          const EventDecoder &eventDecoder,
                                                          Crypto &crypto) {
  const auto &version = FileUtil::readStrFromFile(ifs);
  SPDLOG_INFO("snapshot file version is {}", version);

  if (version == "v1") {
    /// encrypted version
    crypto.beginDecryption(100 * (1 << 20));  // 100MB
    auto offset = crypto.decryptUint64FromFile(ifs);
    auto createdTime = crypto.decryptUint64FromFile(ifs);
    mValue = crypto.decryptUint64FromFile(ifs);
    crypto.commitDecryption(ifs);

    return offset;
  } else {
    SPDLOG_INFO("Cannot recognize version {}", version);
    return std::nullopt;
  }
}

}  /// namespace v1
}  /// namespace demo
}  /// namespace gringofts

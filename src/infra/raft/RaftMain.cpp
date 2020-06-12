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

#include <memory>
#include <unistd.h>

#include <spdlog/sinks/stdout_sinks.h>

#include "../util/CryptoUtil.h"
#include "RaftLogStore.h"
#include "RaftBuilder.h"

struct DummyCommand final : gringofts::Command {
  std::string mPayload;

  explicit DummyCommand(std::string payload) :
      gringofts::Command(0, 0), mPayload(payload) {}

  std::string encodeToString() const override {
    return mPayload;
  }

  void decodeFromString(std::string_view payload) override {
    mPayload = payload;
  }

  void onPersisted(const std::string &message) override {}

  void onPersistFailed(const std::string &errorMessage, std::optional<uint64_t> reserved) override {}
};

struct DummyEvent final : gringofts::Event {
  std::string mPayload;

  explicit DummyEvent(std::string payload) :
      gringofts::Event(0, 0), mPayload(payload) {}

  std::string encodeToString() const override {
    return mPayload;
  }

  void decodeFromString(std::string_view payload) override {
    mPayload = payload;
  }
};

int main(int argc, char *argv[]) {
  spdlog::stdout_logger_mt("console");
  spdlog::set_pattern("[%D %H:%M:%S.%F] [%s:%# %!] [%l] [thread %t] %v");

  /// create raft impl
  auto raftImpl = gringofts::raft::buildRaftImpl(argv[1], std::nullopt);

  /// create fake CryptoUtil
  auto crypto = std::make_shared<gringofts::CryptoUtil>();

  /// create raft log store
  gringofts::raft::RaftLogStore logStore(raftImpl, crypto);

  std::string payload(1000, 'A');   /// 1k
  auto command = std::make_shared<DummyCommand>(payload);
  std::vector<std::shared_ptr<gringofts::Event>> events{std::make_shared<DummyEvent>(payload)};

  uint64_t prevTerm = 0;
  uint64_t lastCommandId = 0;

  bool running = true;
  while (running) {
    usleep(100 * 1000);   /// 100ms

    logStore.refresh();

    auto logStoreRole = logStore.getLogStoreRole();
    auto logStoreTerm = logStore.getLogStoreTerm();

    if (logStoreRole != gringofts::raft::RaftRole::Leader) {
      continue;
    }

    if (prevTerm != logStoreTerm) {
      prevTerm = logStoreTerm;
      lastCommandId = raftImpl->getLastLogIndex();
    }

    SPDLOG_INFO("LogStoreTerm={}, LogStoreRole=LEADER", logStoreTerm);

    for (std::size_t i = 0; i < 20; ++i) {
      logStore.persistAsync(command, events, ++lastCommandId, nullptr);
    }
  }
}

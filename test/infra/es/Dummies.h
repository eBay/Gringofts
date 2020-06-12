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

#ifndef TEST_INFRA_ES_DUMMIES_H_
#define TEST_INFRA_ES_DUMMIES_H_

#include "../../../src/infra/es/Command.h"
#include "../../../src/infra/es/CommandDecoder.h"
#include "../../../src/infra/es/Event.h"
#include "../../../src/infra/es/EventDecoder.h"

namespace gringofts {

struct DummyCommand final : Command {
  std::string mPayload;

  DummyCommand(TimestampInNanos timestamp, std::string payload) :
      Command(0, timestamp), mPayload(payload) {}

  std::string encodeToString() const override {
    return mPayload;
  }

  void decodeFromString(std::string_view payload) override {
    mPayload = payload;
  }

  void onPersisted(const std::string &message) override {}

  void onPersistFailed(const std::string &errorMessage, std::optional<uint64_t> reserved) override {}

  static std::unique_ptr<Command> createDummyCommand() {
    auto timestamp = TimeUtil::currentTimeInNanos();
    auto command = std::make_unique<DummyCommand>(timestamp, "DummyCommand");
    command->setCreatorId(1);
    command->setGroupId(2);
    command->setGroupVersion(3);
    command->setTrackingContext("dummy tracking context");
    command->setRequestHandle(nullptr);

    return command;
  }
};

struct DummyEvent final : Event {
  std::string mPayload;

  DummyEvent(TimestampInNanos timestamp, std::string payload) :
      Event(0, timestamp), mPayload(payload) {}

  std::string encodeToString() const override {
    return mPayload;
  }

  void decodeFromString(std::string_view payload) override {
    mPayload = payload;
  }

  static std::unique_ptr<Event> createDummyEvent() {
    auto timestamp = TimeUtil::currentTimeInNanos();
    auto event = std::make_unique<DummyEvent>(timestamp, "DummyEvent");
    event->setCreatorId(1);
    event->setGroupId(2);
    event->setGroupVersion(3);
    event->setTrackingContext("dummy tracking context");

    return std::move(event);
  }

  static std::vector<std::shared_ptr<Event>> createDummyEvents(uint32_t count) {
    auto timestamp = TimeUtil::currentTimeInNanos();
    std::vector<std::shared_ptr<Event>> events;
    for (auto i = 0; i < count; i++) {
      auto event = createDummyEvent();
      events.push_back(std::move(event));
    }

    return std::move(events);
  }
};

class DummyCommandDecoder : public CommandDecoder {
 public:
  std::unique_ptr<Command> decodeCommandFromString(
      const CommandMetaData &metaData, std::string_view payload) const override {
    auto command = std::make_unique<DummyCommand>(metaData.getCreatedTimeInNanos(), std::string(payload));
    command->setPartialMetaData(metaData);

    return std::move(command);
  }
};

class DummyEventDecoder : public EventDecoder {
 public:
  std::unique_ptr<Event> decodeEventFromString(
      const EventMetaData &metaData, std::string_view payload) const override {
    auto event = std::make_unique<DummyEvent>(metaData.getCreatedTimeInNanos(), std::string(payload));
    event->setPartialMetaData(metaData);

    return std::move(event);
  }
};

}  /// namespace gringofts

#endif  // TEST_INFRA_ES_DUMMIES_H_

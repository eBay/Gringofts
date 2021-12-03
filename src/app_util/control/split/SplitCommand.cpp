/************************************************************************
Copyright 2019-2021 eBay Inc.
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

#include "SplitCommand.h"

#include <unordered_set>
#include <absl/strings/str_format.h>
#include "SplitEvent.h"

namespace gringofts::app::ctrl::split {

std::string SplitCommand::verifyCommand() const {
  if (mRequest.planid().empty()) {
    return "planId is empty";
  }
  if (mRequest.epoch() <= 0) {
    return "epoch should greater than 0";
  }
  std::unordered_map<RouteType, uint64_t> groupTotals;
  std::unordered_map<RouteType, std::unordered_set<uint64_t>> groups;
  for (const auto &impact : mRequest.impacts()) {
    const auto &route = impact.route();
    auto type = route.type();
    if (groupTotals[type] > 0 && groupTotals[type] != route.grouptotal()) {
      return "group total should be same with " + std::to_string(groupTotals[type]);
    } else {
      groupTotals[type] = route.grouptotal();
    }
    for (auto group : route.groups()) {
      if (groups[type].count(group) > 0) {
        return "duplicated group :" + std::to_string(group) + "with type " + std::to_string(type);
      } else {
        groups[type].insert(group);
      }
    }
  }
  return kVerifiedSuccess;
}

ProcessHint SplitCommand::process(const CtrlState &state,
                                  std::vector<std::shared_ptr<Event>> *events) const {
  SPDLOG_INFO("process split event");
  TimestampInNanos time = TimeUtil::currentTimeInNanos();
  if (state.hasState()) {
    auto epoch = state.epoch();
    if (mRequest.epoch() < epoch) {
      return ProcessHint{400, absl::StrFormat("epoch should be greater than %d", epoch)};
    } else if (mRequest.epoch() == epoch) {
      if (mRequest.planid() == state.planId()) {
        return ProcessHint{201, "deduped"};
      } else {
        return ProcessHint{400, absl::StrFormat("plan id should be %s", state.planId())};
      }
    }
  }
  events->push_back(std::make_unique<SplitEvent>(time, mRequest));
  return ProcessHint{200, "ok"};
}

}  // namespace gringofts::app::ctrl::split

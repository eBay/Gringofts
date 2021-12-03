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

#include "SplitEvent.h"

#include "../../AppInfo.h"

namespace gringofts::app::ctrl::split {

void SplitEvent::apply(CtrlState *state) const {
  SPDLOG_INFO("before apply split event state is \n {}", state->prettyPrint());
  bool impacted = false;
  state->clearRoutes();
  for (auto &impact : mRequest.impacts()) {
    if (impact.clusterid() == AppInfo::groupId()) {
      state->applyRoute(impact.route());
      state->setClusterId(impact.clusterid());
      if (impact.newcluster()) {
        auto newStartIndex = getCommandId() + 1;
        SPDLOG_INFO("this cluster {} is a new cluster, reset start index to {}",
                    impact.clusterid(), newStartIndex);
        // only cluster x>0 should be the new cluster
        assert(impact.clusterid());
        // this command is last one from old cluster
        // set start is it + 1
        state->setStartIndex(newStartIndex);
      }
      // has been impacted by this request
      impacted = true;
    }
  }
  // groups has been updated
  if (impacted) {
    state->setEpoch(mRequest.epoch());
    state->setPlanId(mRequest.planid());
    SPDLOG_INFO("after apply split event state is \n {}", state->prettyPrint());
    getGauge("epoch_gauge", {}).set(mRequest.epoch());
  } else {
    SPDLOG_INFO("NO Impact");
  }
  SPDLOG_INFO("applied split event");
}
}  // namespace gringofts::app::ctrl::split

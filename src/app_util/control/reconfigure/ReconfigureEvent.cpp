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

#include "ReconfigureEvent.h"

#include "../../../infra/raft/RaftSignal.h"
#include "../../../infra/forward/ForwardSignal.h"
#include "../../AppStateMachine.h"

namespace gringofts::app::ctrl::reconfigure {

void ReconfigureEvent::syncFromProto() {
  mClusterVersion = mRequest.version();
  mClusterConfiguration.syncFromProto(mRequest.configuration());
}

void ReconfigureEvent::apply(CtrlState *state) const {
  SPDLOG_INFO("before apply RECONFIGURE event, the ctrl state is: \n {}", state->prettyPrint());
  state->setClusterConfiguration(mClusterVersion, mClusterConfiguration, getCommandId());
  SPDLOG_INFO("after apply RECONFIGURE event, the ctrl state is: \n {}", state->prettyPrint());
}

void ReconfigureEvent::onApplied() const {
  if (mClusterVersion <= AppInfo::getClusterVersion()) {
    return;
  }

  NodeId newSelfId = kUnknownNodeId;
  NodeId oldSelfId = AppInfo::getMyNodeId();
  std::string myHostname;

  mClusterConfiguration.deduceSelfNodeId(&newSelfId, &myHostname);
  SPDLOG_INFO("ReconfigureEvent onApplied, oldSelfId={}, newSelfId={}", oldSelfId, newSelfId);

  // 1. update cluster info in AppInfo
  AppInfo::setClusterInfo(mClusterVersion, newSelfId, mClusterConfiguration);
  // 2. notify raft to reconfigure
  Signal::hub << std::make_shared<raft::ReconfigureSignal>(mClusterVersion, newSelfId,
                                                           mClusterConfiguration);
  // 3. notify forward core to update
  Signal::hub << std::make_shared<forward::ForwardReconfigureSignal>(mClusterConfiguration);
}

}  // namespace gringofts::app::ctrl::reconfigure

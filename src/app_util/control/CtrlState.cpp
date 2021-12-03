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

#include "CtrlState.h"

#include <sstream>
#include <spdlog/spdlog.h>

#include "../AppInfo.h"
#include "../../infra/util/ClusterInfo.h"
#include "../../infra/util/Signal.h"
#include "../../infra/raft/RaftSignal.h"

namespace gringofts::app::ctrl {
void CtrlState::applyRoute(const RouteProto &proto) {
  mRouteMap.applyRoute(proto.type(), Route(proto));
}

SplitState CtrlState::buildProto() const {
  SplitState state;
  state.set_epoch(mEpoch);
  state.set_planid(mPlanId);
  state.set_clusterid(mClusterId);
  state.set_startindex(mStartIndex);
  *state.mutable_routes() = mRouteMap.buildProto();
  return state;
}

std::string CtrlState::prettyPrint() const {
  if (hasState()) {
    std::stringstream ss;
    ss << "--- control state --- " << std::endl;
    ss << mRouteMap.prettyPrint() << std::endl;
    ss << "Epoch:\t\t" << mEpoch << std::endl;
    ss << "PlanId:\t\t" << mPlanId << std::endl;
    ss << "ClusterId:\t" << mClusterId << std::endl;
    ss << "StartIndex:\t" << mStartIndex;
    return ss.str();
  }
  return "<empty state>";
}

std::string CtrlState::encodeToString() const {
  if (mEpoch >= 0) {
    auto state = buildProto();
    return state.SerializeAsString();
  } else {
    return "";
  }
}
void CtrlState::decodeFromString(std::string_view view) {
  if (!view.empty()) {
    SplitState decodeState;
    decodeState.ParseFromString(std::string(view));
    if (decodeState.clusterid() == AppInfo::groupId()) {
      mRouteMap.parseFromProto(decodeState.routes());
      mPlanId = decodeState.planid();
      mClusterId = decodeState.clusterid();
      mEpoch = decodeState.epoch();
      mStartIndex = decodeState.startindex();
    } else {
      SPDLOG_WARN("Loaded state but for cluster {}, this cluster {} has no state",
                  decodeState.clusterid(),
                  AppInfo::groupId());
    }
  }
}

void CtrlState::recoverForEAL(std::string_view str) {
  decodeFromString(str);
  SPDLOG_INFO("load ctrl state: {}", prettyPrint());
  // for cluster Id > 0, need to start raft
  // for cluster = 0, direct start raft
  if (hasState() && mClusterId > 0) {
    auto routeSignal = std::make_shared<gringofts::RouteSignal>(mEpoch, mClusterId);
    // query route info to guarantee it can start raft
    Signal::hub << routeSignal;
    if (routeSignal->getFuture().get()) {
      SPDLOG_INFO("start raft");
      // has groups in this cluster
      // send signal to tell raft to become follower
      Signal::hub << std::make_shared<raft::StopSyncRoleSignal>(mStartIndex);
    } else {
      SPDLOG_INFO("stopped start raft");
    }
  }
  getGauge("epoch_gauge", {}).set(mEpoch);
}
}  // namespace gringofts::app::ctrl

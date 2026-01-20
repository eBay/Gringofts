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

#ifndef SRC_APP_UTIL_CONTROL_CTRLSTATE_H_
#define SRC_APP_UTIL_CONTROL_CTRLSTATE_H_

#include "../AppInfo.h"
#include "../generated/grpc/ctrl.pb.h"
#include "../../infra/Decodable.h"
#include "../../infra/Encodable.h"
#include "Route.h"

namespace gringofts::app::ctrl {

using protos::SplitState;
using RouteProto = protos::Route;

class CtrlState : public Encodable, public Decodable {
 public:
  typedef protos::SplitState SplitState;
  typedef protos::Route RouteProto;
  typedef protos::ClusterConfiguration ClusterConfigurationProto;

  CtrlState() = default;

  // note that, thread safe properties are not copied
  CtrlState(const CtrlState& obj) {
    mRouteMap = obj.mRouteMap;
    mEpoch = obj.mEpoch;
    mPlanId = obj.mPlanId;
    mClusterId = obj.mClusterId;
    mStartIndex = obj.mStartIndex;
    mClusterVersion = obj.mClusterVersion;
    mClusterConfiguration = obj.mClusterConfiguration;
    mClusterReconfigureIndex = obj.mClusterReconfigureIndex;
  }

  CtrlState& operator=(const CtrlState& obj) {
    if (this == &obj) {
      return *this;
    }
    mRouteMap = obj.mRouteMap;
    mEpoch = obj.mEpoch;
    mPlanId = obj.mPlanId;
    mClusterId = obj.mClusterId;
    mStartIndex = obj.mStartIndex;
    mClusterVersion = obj.mClusterVersion;
    mClusterConfiguration = obj.mClusterConfiguration;
    mClusterReconfigureIndex = obj.mClusterReconfigureIndex;
    return *this;
  }

  inline uint64_t epoch() const { return mEpoch; }

  inline std::string_view planId() const { return mPlanId; }

  inline uint64_t startIndex() const { return mStartIndex; }

  inline uint64_t clusterId() const { return mClusterId; }
  inline uint64_t clusterVersion() const { return mClusterVersion; }
  inline const ClusterInfo& clusterConfiguration() const { return mClusterConfiguration; }
  inline uint64_t clusterReconfigureIndex() const { return mClusterReconfigureIndex; }

  /**
   * if epoch > 0, we have split before, route map can be used
   * else epoch <= 0 we don't have any split before, it will serve all traffic
   * @return
   */
  inline bool hasSplitState() const { return mEpoch > 0; }

  inline void setEpoch(uint64_t epoch) { mEpoch = epoch; }

  inline void setPlanId(const std::string &planId) { mPlanId = planId; }

  inline void setClusterId(uint64_t clusterId) { mClusterId = clusterId; }

  inline void setStartIndex(uint64_t startIndex) { mStartIndex = startIndex; }
  inline bool hasClusterConfiguration() const { return mClusterVersion > 0; }

  inline void setClusterConfiguration(uint64_t clusterVersion,
                                      const ClusterInfo &clusterConfiguration,
                                      uint64_t clusterSwitchIndex) {
    mClusterVersion = clusterVersion;
    mClusterConfiguration = clusterConfiguration;
    mClusterReconfigureIndex = clusterSwitchIndex;
  }

  inline void clearRoutes() { mRouteMap.clear(); }

  inline bool hasGroup(RouteType type, uint64_t group) const { return mRouteMap.hasGroup(type, group); }

  inline uint64_t groupTotal(RouteType type) const { return mRouteMap.groupTotal(type); }

  void applyRoute(const RouteProto &proto);

  SplitState buildProto() const;

  std::string prettyPrint() const;

  std::string encodeToString() const override;

  void decodeFromString(std::string_view view) override;

  void recoverForEAL(std::string_view str);

 private:
  RouteMap mRouteMap;
  uint64_t mEpoch = 0;
  std::string mPlanId;
  uint64_t mClusterId;
  uint64_t mStartIndex;
  // state for cluster configuration:
  uint64_t mClusterVersion = 0;
  ClusterInfo mClusterConfiguration;
  uint64_t mClusterReconfigureIndex = 0;  // raft index of the reconfigure command
};
}  // namespace gringofts::app::ctrl

#endif  // SRC_APP_UTIL_CONTROL_CTRLSTATE_H_

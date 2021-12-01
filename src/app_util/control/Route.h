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

#ifndef SRC_APP_UTIL_CONTROL_ROUTE_H_
#define SRC_APP_UTIL_CONTROL_ROUTE_H_

#include <unordered_set>
#include "../generated/grpc/route.pb.h"

namespace gringofts::app::ctrl {
using RouteType = uint32_t;
using RouteProto = protos::Route;
using RouteProtoList = ::google::protobuf::RepeatedPtrField<RouteProto>;

class Route {
 public:
  Route() = default;

  explicit Route(const RouteProto &proto);

  RouteProto buildProto() const;

  inline RouteType routeType() const { return mType; }
  inline uint64_t groupTotal() const { return mGroupTotal; }
  inline const std::unordered_set<uint64_t> &groups() const { return mGroups; }

 private:
  std::unordered_set<uint64_t> mGroups;
  uint64_t mGroupTotal;
  RouteType mType;
};

class RouteMap {
 public:
  RouteMap() = default;

  inline bool empty() const { return mMap.empty(); }

  inline void clear() { mMap.clear(); }

  void parseFromProto(const RouteProtoList &repeatRoutes);

  uint64_t groupTotal(RouteType type) const;

  bool hasGroup(RouteType type, uint64_t group) const;

  void applyRoute(RouteType type, Route route);

  RouteProtoList buildProto() const;

  std::string prettyPrint() const;

 private:
  std::unordered_map<RouteType , Route> mMap;
};

}  // namespace gringofts::app::ctrl
#endif  // SRC_APP_UTIL_CONTROL_ROUTE_H_

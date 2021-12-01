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

#include "Route.h"

#include <sstream>
#include <spdlog/spdlog.h>

namespace gringofts::app::ctrl {

/// Route definition
Route::Route(const RouteProto &proto) :
    mGroupTotal(proto.grouptotal()), mType(proto.type()) {
  for (auto group : proto.groups()) {
    mGroups.insert(group);
  }
}

RouteProto Route::buildProto() const {
  RouteProto proto;
  proto.set_type(mType);
  proto.set_grouptotal(mGroupTotal);
  for (auto group : mGroups) {
    proto.mutable_groups()->Add(group);
  }
  return proto;
}

/// RouteMap definition
void RouteMap::parseFromProto(const RouteProtoList &repeatRoutes) {
  for (const auto &routeProto : repeatRoutes) {
    mMap[routeProto.type()] = std::move(Route{routeProto});
  }
}

uint64_t RouteMap::groupTotal(RouteType type) const {
  auto it = mMap.find(type);
  if (it == mMap.end()) {
    return 0;
  }
  return it->second.groupTotal();
}

bool RouteMap::hasGroup(RouteType type, uint64_t group) const {
  auto it = mMap.find(type);
  if (it == mMap.end()) {
    return false;
  }
  auto &groups = it->second.groups();
  return groups.find(group) != groups.end();
}

void RouteMap::applyRoute(RouteType type, Route route) {
  if (route.groups().empty()) {
    if (mMap.find(type) != mMap.end()) {
      SPDLOG_INFO("{} type groups is empty, remove the old route", route.routeType());
      mMap.erase(type);
    }
  } else {
    mMap[type] = std::move(route);
  }
}

RouteProtoList RouteMap::buildProto() const {
  RouteProtoList list;
  for (const auto &routeKV : mMap) {
    *list.Add() = routeKV.second.buildProto();
  }
  return list;
}

std::string RouteMap::prettyPrint() const {
  std::stringstream ss;
  for (const auto &routeKV : mMap) {
    const auto &route = routeKV.second;
    ss << "RouterType:\t" << route.routeType() << std::endl;
    ss << "Groups:\t\t[";
    for (auto &group : route.groups()) {
      ss << group << ", ";
    }
    ss << "]" << std::endl;
    ss << "GroupTotal:\t" << route.groupTotal() << std::endl;
  }
  return ss.str();
}

}  // namespace gringofts::app::ctrl

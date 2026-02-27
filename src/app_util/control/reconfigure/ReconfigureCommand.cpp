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

#include "ReconfigureCommand.h"

#include "ReconfigureEvent.h"
#include "../../../infra/util/HttpCode.h"

#include <set>
#include <map>
#include <absl/strings/str_cat.h>

namespace gringofts::app::ctrl::reconfigure {

std::string ReconfigureCommand::verifyCommand() const {
  if (mRequest.configuration().clusterid() != getGroupId()) {
    std::string errMsg = absl::StrCat("This reconfigure command is sent to a wrong cluster. "
      "cluster id in request: ", std::to_string(mRequest.configuration().clusterid()), ", id of this cluster: ",
      std::to_string(getGroupId()));
    return errMsg;
  }

  std::set<uint64_t> memberIdSet, jointMemberIdSet;
  const auto &members = mRequest.configuration().members();
  const auto &jointMemberIds = mRequest.configuration().jointmembers();

  // verify members
  if (members.size() == 0) {
    return "Members in reconfigure command should not be empty.";
  }

  // verify member id: should be > 0
  for (const auto &member : members) {
    if (member.id() == 0) {
      return "Member id should be greater than 0.";
    }
  }

  // verify joint members
  if (jointMemberIds.newmemberids_size() == 0 && jointMemberIds.oldmemberids_size() != 0) {
    return "New members should not be empty when old members are not empty.";
  } else if (jointMemberIds.newmemberids_size() != 0 && jointMemberIds.oldmemberids_size() == 0) {
    return "Old members should not be empty when new members are not empty.";
  } else if (jointMemberIds.newmemberids_size() != 0 && jointMemberIds.oldmemberids_size() != 0) {
    for (const auto &member : members) {
      memberIdSet.insert(member.id());
    }
    for (const auto &id : jointMemberIds.newmemberids()) {
      if (memberIdSet.find(id) == memberIdSet.end()) {
        std::string errMsg = absl::StrCat("New member id '", std::to_string(id), "' is not in members list.");
        return errMsg;
      }
      jointMemberIdSet.insert(id);
    }
    for (const auto &id : jointMemberIds.oldmemberids()) {
      if (memberIdSet.find(id) == memberIdSet.end()) {
        std::string errMsg = absl::StrCat("Old member id '", std::to_string(id), "' is not in members list.");
        return errMsg;
      }
      jointMemberIdSet.insert(id);
    }
    if (jointMemberIdSet.size() != members.size()) {
      return "Joint member size should be the same as member's.";
    }
  }

  return kVerifiedSuccess;
}

ProcessHint ReconfigureCommand::process(const CtrlState &state,
                                        std::vector<std::shared_ptr<Event>> *events) const {
  TimestampInNanos time = TimeUtil::currentTimeInNanos();

  // cluster info in state:
  const auto &configurationInState = state.clusterConfiguration();
  const auto &versionInState = state.clusterVersion();
  // cluster info in appInfo:
  const auto &configureInAppInfo = AppInfo::getMyClusterInfo();
  const auto &verionInAppInfo = AppInfo::getClusterVersion();

  // check version:
  // new version should be current version + 1
  auto latestVersion = std::max(versionInState, verionInAppInfo);
  if (mRequest.version() != latestVersion + 1) {
    std::string errMsg = absl::StrCat("Version should be current version + 1. current version: ",
                                      std::to_string(latestVersion),
                                      ", new version: ", std::to_string(mRequest.version()));
    return ProcessHint{HttpCode::BAD_REQUEST, errMsg};
  }

  // check members
  // new members should have instersection with current members.
  std::map<std::string, NodeId> addresses;
  const auto &latestConfiguration = versionInState > verionInAppInfo ? configurationInState : configureInAppInfo;
  SPDLOG_INFO("Apply reconfigure command, latest configuration: {}, request: {}",
              latestConfiguration.to_string(), mRequest.ShortDebugString());
  for (const auto &node : latestConfiguration.getAllNodeInfo()) {
    addresses[node.second.mHostName + ":" + std::to_string(node.second.mPortForRaft)] = node.first;
  }
  bool hasIntersection = false;
  for (const auto &member : mRequest.configuration().members()) {
    auto port = member.port() == 0 ? kDefaultRaftPort : member.port();
    if (addresses.find(member.address() + ":" + std::to_string(port)) != addresses.end()) {
      hasIntersection = true;
      break;
    }
  }
  if (!hasIntersection) {
    return ProcessHint{HttpCode::BAD_REQUEST, "New members should have intersection with current members."};
  }

  // new node should not be voter
  for (const auto &member : mRequest.configuration().members()) {
    auto port = member.port() == 0 ? kDefaultRaftPort : member.port();
    auto it = addresses.find(member.address() + ":" + std::to_string(port));
    if (it == addresses.end() && member.role() == protos::InitialRaftRole::Voter) {
      std::string errMsg = absl::StrCat("New members should not be voter. ", member.id(), "@",
                                         member.address(), ":", port, " is voter.");
      return ProcessHint{HttpCode::BAD_REQUEST, errMsg};
    }
  }

  // only non-voter can be removed
  std::set<std::string> addressesFromRequest;
  for (const auto &member : mRequest.configuration().members()) {
    auto port = member.port() == 0 ? kDefaultRaftPort : member.port();
    addressesFromRequest.insert(member.address() + ":" + std::to_string(port));
  }
  const auto initialRoles = latestConfiguration.getAllInitialRoles();
  for (const auto &node : latestConfiguration.getAllNodeInfo()) {
    if (addressesFromRequest.find(node.second.mHostName + ":" + std::to_string(node.second.mPortForRaft))
        == addressesFromRequest.end() && (initialRoles.find(node.first) == initialRoles.end() ||
        initialRoles.at(node.first) == protos::InitialRaftRole::Voter)) {
      std::string errMsg = absl::StrCat("Only non-voter can be removed from cluster. ", node.first, "@",
                                         node.second.mHostName, ":", node.second.mPortForRaft, " is voter.");
      return ProcessHint{HttpCode::BAD_REQUEST, errMsg};
    }
  }

  // check hostname
  // hostname's id cannot be changed
  for (const auto &member : mRequest.configuration().members()) {
    auto port = member.port() == 0 ? kDefaultRaftPort : member.port();
    auto it = addresses.find(member.address() + ":" + std::to_string(port));
    if (it != addresses.end() && it->second != member.id()) {
      std::string errMsg = absl::StrCat("Node id of '", it->first, "' changes: ", std::to_string(it->second),
                                        "->", std::to_string(member.id()));
      return ProcessHint{HttpCode::BAD_REQUEST, errMsg};
    }
  }

  // verfify in-sync learners are sub-set of learners
  std::set<uint64_t> learnerSet;
  for (const auto &member : mRequest.configuration().members()) {
    if (member.role() == protos::InitialRaftRole::Learner) {
      learnerSet.insert(member.id());
    }
  }
  for (const auto &id : mRequest.configuration().insynclearners()) {
    if (learnerSet.find(id) == learnerSet.end()) {
      std::string errMsg = absl::StrCat("In-sync learner id '", std::to_string(id), "' is not in learner list.");
      return ProcessHint{HttpCode::BAD_REQUEST, errMsg};
    }
  }

  // generate events
  events->push_back(std::make_unique<ReconfigureEvent>(time, mRequest));
  for (auto &eventPtr : *events) {
    eventPtr->setCreatorId(gringofts::app::AppInfo::subsystemId());
    eventPtr->setGroupId(gringofts::app::AppInfo::groupId());
    eventPtr->setGroupVersion(gringofts::app::AppInfo::groupVersion());
  }

  return ProcessHint{HttpCode::OK, "Success"};
}

}  // namespace gringofts::app::ctrl::reconfigure

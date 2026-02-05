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

#pragma once

#include <string>
#include <INIReader.h>

#include "../util/ClusterInfo.h"
#include "ForwardClient.h"
#include "ForwardSignal.h"

namespace gringofts {
namespace forward {

struct Peer {
  uint64_t mId = 0;
  std::string mAddress;
  std::atomic<uint64_t> mPointer;
};

template <typename StubType>
class ForwardCore {
 public:
  ForwardCore() {}
  ~ForwardCore() {}

  void init(const INIReader &reader, const NodeId &myNodeId,
      const ClusterInfo &clusterInfo, std::shared_ptr<DNSResolver> dnsResolver = nullptr,
      uint32_t customPort = 0) {
    mSelfId = myNodeId;
    mCustomPort = customPort;
    mTlsConfOpt = TlsUtil::parseTlsConf(reader, "tls");
    mDNSResolver = dnsResolver;
    if (mDNSResolver == nullptr) {
      /// use default dns resolver
      mDNSResolver = std::make_shared<DNSResolver>();
    }
    initClusterConf(clusterInfo);
    initClients();
    Signal::hub.handle<ForwardReconfigureSignal>([this](const Signal &s) {
      const auto &signal = dynamic_cast<const ForwardReconfigureSignal &>(s);
      SPDLOG_INFO("ForwardCore receive reconfigure signal, cluster configuration: {}",
        signal.getClusterConfiguration().to_string());
      update(signal.getClusterConfiguration());
    });
  }

  // reconfiguration may happen, so we need to add the peers
  // we only add peers for nonexisting nodes for forwardcore rathan than rebuilding all
  void update(const ClusterInfo &clusterInfo) {
    std::unique_lock lock(mMutex);
    auto nodes = clusterInfo.getAllNodeInfo();
    for (auto &[nodeId, node] : nodes) {
      if (mPeers.find(nodeId) == mPeers.end()) {
        if (nodeId == mSelfId) {
          continue;
        }
        std::string host = node.mHostName;
        std::string port = mCustomPort > 0 ? std::to_string(mCustomPort) : std::to_string(node.mPortForGateway);
        std::string addr = host + ":" + port;

        mPeers[nodeId].mId = nodeId;
        mPeers[nodeId].mAddress = addr;
        mPeers[nodeId].mPointer.store(0);
        SPDLOG_INFO("Add peer mId {}, mAddress {}", nodeId, addr);
        for (int i = 0; i < mConcurrency; ++i) {
          auto clientId = nodeId * mConcurrency + i;
          mClients[clientId] = std::make_unique<ForwardClientBase<StubType>>(
              addr, mTlsConfOpt, mDNSResolver, mClusterId, nodeId, clientId);
        }
      }
    }
  }

  template<typename RequestType, typename RpcFuncType, typename CallType>
  bool forwardRequest(std::shared_ptr<RequestType> request, RpcFuncType rpcFunc, CallType *call) {
    if (call == nullptr || call->mMeta == nullptr) {
      SPDLOG_ERROR("call or call->mMeta is nullptr");
      return false;
    }
    if (rpcFunc == nullptr || request == nullptr) {
      SPDLOG_ERROR("rpcFunc or request is nullptr");
      return false;
    }
    if (mPeers.find(call->mMeta->mLeaderId) == mPeers.end()) {
      SPDLOG_ERROR("mLeaderID not legal", call->mMeta->mLeaderId);
      return false;
    }
    auto &peer = mPeers[call->mMeta->mLeaderId];
    auto clientIndex = (peer.mId * mConcurrency) + (peer.mPointer.fetch_add(1) % mConcurrency);
    auto client = mClients.find(clientIndex);
    if (client == mClients.end() || client->second == nullptr) {
      SPDLOG_ERROR("ForwardClient for peer {} not found", peer.mId);
      return false;
    }
    SPDLOG_DEBUG("Forward Request to {}", peer.mId);
    return client->second->forwardRequest(request, rpcFunc, call);
  }

 private:
  void initClusterConf(const ClusterInfo &clusterInfo) {
    mClusterId = clusterInfo.getClusterId();
    auto nodes = clusterInfo.getAllNodeInfo();
    std::unique_lock lock(mMutex);
    for (auto &[nodeId, node] : nodes) {
      std::string host = node.mHostName;
      std::string port = mCustomPort > 0 ? std::to_string(mCustomPort) : std::to_string(node.mPortForGateway);
      std::string addr = host + ":" + port;

      if (nodeId != mSelfId) {
        mPeers[nodeId].mId = nodeId;
        mPeers[nodeId].mAddress = addr;
        mPeers[nodeId].mPointer.store(0);
        SPDLOG_INFO("Add peer mId {}, mAddress {}", nodeId, addr);
      }
    }
    SPDLOG_INFO("cluster.size={}, self.id={}", mPeers.size() + 1, mSelfId);
  }

  void initClients() {
    std::unique_lock lock(mMutex);
    for (auto &[peerId, peer] : mPeers) {
      for (int i = 0; i < mConcurrency; ++i) {
        auto clientId = peerId * mConcurrency + i;
        mClients[clientId] = std::make_unique<ForwardClientBase<StubType>>(
            peer.mAddress, mTlsConfOpt, mDNSResolver, mClusterId, peerId, clientId);
      }
    }
  }

 private:
  std::map<uint64_t, std::unique_ptr<ForwardClientBase<StubType>>> mClients;
  // use multiple clients for one pu node to reach max tps
  const uint64_t mConcurrency = 3;
  // to protect mPeers, mClients, may be updated by reconfiguration
  mutable std::shared_mutex mMutex;
  std::map<uint64_t, Peer> mPeers;
  uint64_t mSelfId;
  ClusterId mClusterId;
  uint32_t mCustomPort = 0;

  std::optional<TlsConf> mTlsConfOpt;
  std::shared_ptr<DNSResolver> mDNSResolver;
};

}  /// namespace forward
}  /// namespace gringofts

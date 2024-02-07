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
      const ClusterInfo &clusterInfo, std::shared_ptr<DNSResolver> dnsResolver = nullptr) {
    mSelfId = myNodeId;
    initClusterConf(clusterInfo);
    if (dnsResolver == nullptr) {
      /// use default dns resolver
      dnsResolver = std::make_shared<DNSResolver>();
    }
    initClients(reader, dnsResolver);
  }

  template<typename RequestType, typename RpcFuncType, typename CallType>
  void forwardRequest(std::shared_ptr<RequestType> request, RpcFuncType rpcFunc, CallType *call) {
    auto &peer = mPeers[call->mMeta->mLeaderId];
    auto index = peer.mPointer.fetch_add(1) % mConcurrency;
    auto &client = mClients[peer.mId * mConcurrency + index];
    client->forwardRequest(request, rpcFunc, call);
    SPDLOG_DEBUG("Forward Request to {}", peer.mId);
  }

 private:
  void initClusterConf(const ClusterInfo &clusterInfo) {
    auto nodes = clusterInfo.getAllNodeInfo();
    for (auto &[nodeId, node] : nodes) {
      std::string host = node.mHostName;
      std::string port = std::to_string(node.mPortForGateway);
      std::string addr = host + ":" + port;

      if (nodeId != mSelfId) {
        mPeers[nodeId].mId = nodeId;
        mPeers[nodeId].mAddress = addr;
        mPeers[nodeId].mPointer.store(0);
        SPDLOG_INFO("Add peer mid {}, mAddress {}", nodeId, addr);
      }
    }
    SPDLOG_INFO("cluster.size={}, self.id={}", mPeers.size() + 1, mSelfId);
  }

  void initClients(const INIReader &reader, std::shared_ptr<DNSResolver> dnsResolver) {
    auto tlsConfOpt = TlsUtil::parseTlsConf(reader, "tls");
    for (auto &[peerId, peer] : mPeers) {
      for (int i = 0; i < mConcurrency; ++i) {
        auto clientId = peerId * mConcurrency + i;
        mClients[clientId] = std::make_unique<ForwardClientBase<StubType>>(
            peer.mAddress, tlsConfOpt, dnsResolver, peerId, clientId);
      }
    }
  }

 private:
  std::map<uint64_t, std::unique_ptr<ForwardClientBase<StubType>>> mClients;
  // use multiple clients for one pu node to reach max tps
  const uint64_t mConcurrency = 3;
  std::map<uint64_t, Peer> mPeers;
  uint64_t mSelfId;
};

}  /// namespace forward
}  /// namespace gringofts

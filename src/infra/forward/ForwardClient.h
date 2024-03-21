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

#include "../util/DNSResolver.h"
#include "../util/TlsUtil.h"
#include "ForwardBase.h"

namespace gringofts {
namespace forward {

template<typename ResponseType>
struct AsyncClientCall : public AsyncClientCallBase {
  ResponseType mResponse;
  std::unique_ptr<grpc::ClientAsyncResponseReader<ResponseType>> mResponseReader;
};

template<typename StubType>
class ForwardClientBase {
 public:
  ForwardClientBase(const std::string &peerHostname,
                    std::optional<TlsConf> tlsConfOpt,
                    std::shared_ptr<DNSResolver> dnsResolver,
                    uint64_t peerId, uint64_t clientId):
      mPeerAddress(peerHostname),
      mTLSConfOpt(tlsConfOpt),
      mDNSResolver(dnsResolver),
      mPeerId(peerId),
      mClientId(clientId),
      mGaugeReplyQueueSize(gringofts::getGauge("forward_reply_queue_size", {{"clientId", std::to_string(clientId)}})) {
    refreshChannel();
    mClientLoop = std::thread(&ForwardClientBase::clientLoopMain, this);
    mReplyLoop = std::thread(&ForwardClientBase::replyLoopMain, this);
  }
  ~ForwardClientBase() {
    mRunning = false;

    /// shut down CQ
    mCompletionQueue.Shutdown();

    /// join event loop.
    if (mClientLoop.joinable()) {
      mClientLoop.join();
    }

    if (mReplyLoop.joinable()) {
      mReplyLoop.join();
    }

    /// drain completion queue.
    void *tag;
    bool ok;
    while (mCompletionQueue.Next(&tag, &ok)) {}
  }

  template<typename RequestType, typename RpcFuncType, typename CallType>
  bool forwardRequest(std::shared_ptr<RequestType> request, RpcFuncType rpcFunc, CallType *call) {
    if (mStub == nullptr) {
      SPDLOG_WARN("ForwardClient for {} is nullptr", mPeerId);
      return false;
    }
    call->mForwardRquestTime = TimeUtil::currentTimeInNanos();
    if (call->mMeta->mServerContext != nullptr) {
      call->mContext.set_deadline(call->mMeta->mServerContext->deadline());
    } else {
      std::chrono::time_point deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
      call->mContext.set_deadline(deadline);
    }

    std::shared_lock<std::shared_mutex> lock(mMutex);
    call->mResponseReader = (mStub.get()->*rpcFunc)(
        &call->mContext,
        *request.get(),
        &mCompletionQueue);
    call->mResponseReader->StartCall();
    call->mResponseReader->Finish(&call->mResponse,
        &call->mStatus,
        reinterpret_cast<void *>(call));
    return true;
  }

 private:
  void refreshChannel() {
    grpc::ChannelArguments chArgs;
    chArgs.SetMaxReceiveMessageSize(INT_MAX);
    chArgs.SetInt("grpc.testing.fixed_reconnect_backoff_ms", 100);
    chArgs.SetString("key_" + std::to_string(mClientId), "value_" + std::to_string(mClientId));
    auto newResolvedAddress = mDNSResolver->resolve(mPeerAddress);
    if (newResolvedAddress != mResolvedPeerAddress) {
      std::unique_lock<std::shared_mutex> lock(mMutex);
      if (newResolvedAddress != mResolvedPeerAddress) {
        SPDLOG_INFO("refreshing channel, addr {}, new resolved addr {}, old resolved addr {}",
                    mPeerAddress, newResolvedAddress, mResolvedPeerAddress);
        auto channel = grpc::CreateCustomChannel(
            newResolvedAddress, TlsUtil::buildChannelCredentials(mTLSConfOpt), chArgs);
        mStub = std::make_unique<StubType>(channel);
        mResolvedPeerAddress = newResolvedAddress;
      }
    }
  }

  void clientLoopMain() {
    auto peerThreadName = std::string("ForwardClient") + std::to_string(mPeerId);
    pthread_setname_np(pthread_self(), peerThreadName.c_str());

    void *tag;  /// The tag is the memory location of the call object
    bool ok = false;

    /// Block until the next result is available in the completion queue.
    while (mCompletionQueue.Next(&tag, &ok)) {
      if (!mRunning) {
        SPDLOG_INFO("Client loop quit.");
        return;
      }

      auto *call = static_cast<AsyncClientCallBase *>(tag);
      GPR_ASSERT(ok);

      if (!call->mStatus.ok()) {
        SPDLOG_WARN("{} failed., gRpc error_code: {}, error_message: {}, error_details: {}",
                    call->toString(),
                    call->mStatus.error_code(),
                    call->mStatus.error_message(),
                    call->mStatus.error_details());

        /// collect gRpc error code metrics
        getCounter("forward_error_counter", {{"error_code", std::to_string(call->mStatus.error_code())}}).increase();
        refreshChannel();
      }
      call->mGotResponseTime = TimeUtil::currentTimeInNanos();
      mReplyQueue.enqueue(call);
      mGaugeReplyQueueSize.set(mReplyQueue.size());
    }
  }

  void replyLoopMain() {
    auto peerThreadName = std::string("ForwardReply") + std::to_string(mPeerId);
    pthread_setname_np(pthread_self(), peerThreadName.c_str());
    while (mRunning) {
      auto call = mReplyQueue.dequeue();
      mGaugeReplyQueueSize.set(mReplyQueue.size());
      call->mReplyResponseTime = TimeUtil::currentTimeInNanos();
      call->handleResponse();
      SPDLOG_DEBUG("ForwardLatency,{},{}",
          call->mGotResponseTime - call->mForwardRquestTime,
          call->mReplyResponseTime - call->mGotResponseTime);
      delete call;
    }
  }

 private:
  std::unique_ptr<StubType> mStub;
  std::shared_mutex mMutex;  /// the lock to guarantee thread-safe access of mStub
  grpc::CompletionQueue mCompletionQueue;
  std::string mPeerAddress;
  std::string mResolvedPeerAddress;
  std::optional<TlsConf> mTLSConfOpt;
  std::shared_ptr<DNSResolver> mDNSResolver;
  uint64_t mPeerId = 0;

  /// flag that notify thread to quit
  std::atomic<bool> mRunning = true;
  std::thread mClientLoop;
  std::thread mReplyLoop;
  BlockingQueue<AsyncClientCallBase*> mReplyQueue;
  uint64_t mClientId;

  mutable santiago::MetricsCenter::GaugeType mGaugeReplyQueueSize;
};

}  /// namespace forward
}  /// namespace gringofts

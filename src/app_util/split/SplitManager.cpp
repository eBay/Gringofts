/*
 * Copyright (c) 2020 eBay Software Foundation. All rights reserved.
 */
#include "SplitManager.h"
namespace gringofts::app::split {
void SplitManager::run() {
  mRequestReceiverThread = std::make_unique<std::thread>([this]() {
    pthread_setname_np(pthread_self(), "SplitReqReceiver");
    mRequestReceiver->run();
  });
}

void SplitManager::shutDown() {
  mRequestReceiver->shutdown();
  mRequestReceiverThread->join();
}

void SplitManager::apply(const SplitEvent& event, ProcessState *state) {
  state->mSplitState = ProcessState::READ_SPLIT_TAG;
}
ProcessState SplitManager::initProcessState(const INIReader &reader) {
  ProcessState state;
  state.mSplitState = ProcessState::PENDING_SPLIT;
  return state;
}
}  // namespace trinidad

/*
 * Copyright (c) 2020 eBay Software Foundation. All rights reserved.
 */
#ifndef SRC_SplitManager_H_
#define SRC_SplitManager_H_

#include <memory>
#include "SplitEvent.h"
#include "../../infra/es/ProcessCommandStateMachine.h"
#include "RequestReceiver.h"

namespace gringofts::app::split {

class SplitManager {
 public:
  SplitManager(
      const INIReader &reader,
      BlockingQueue<std::shared_ptr<Command>> &blocking_queue
  )
      : mRequestReceiver(std::make_unique<RequestReceiver>(reader, blocking_queue)) {}
  void run();
  void shutDown();
  template<typename StateMachine>
  static void initWithSplitState(const INIReader &reader, StateMachine &stateMachine) {
    stateMachine.initProcessState(initProcessState(reader));
  }
  template<typename StateMachine>
  static void wrapper(const Event &event, StateMachine &stateMachine) {
    if (event.getType() == split::SPILT_EVENT) {
      ProcessState state = stateMachine.readProcessState();
      split::SplitManager::apply(dynamic_cast<const split::SplitEvent &>(event), &state);
      stateMachine.writeProcessState(state);
    } else {
      stateMachine.applyEvent(event);
    }
  }
 private:
  static void apply(const SplitEvent &event, ProcessState *state);
  static ProcessState initProcessState(const INIReader &reader);
  std::unique_ptr<RequestReceiver> mRequestReceiver;
  std::unique_ptr<std::thread> mRequestReceiverThread;
};

}  // namespace gringofts::app::split
#endif  // SRC_SplitManager_H_

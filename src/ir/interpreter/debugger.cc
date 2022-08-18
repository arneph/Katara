//
//  debugger.cc
//  Katara
//
//  Created by Arne Philipeit on 8/14/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "debugger.h"

#include "src/common/logging/logging.h"

namespace ir_interpreter {

Debugger::ExecutionState Debugger::execution_state() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return exec_state_;
}

int64_t Debugger::exit_code() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (exec_state_ != ExecutionState::kTerminated) {
    common::fail("program has not terminated");
  }
  return exit_code_.value();
}

const Stack& Debugger::stack() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (exec_state_ != ExecutionState::kPaused) {
    common::fail("program is not paused");
  }
  return stack_;
}

const Heap& Debugger::heap() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (exec_state_ != ExecutionState::kPaused) {
    common::fail("program is not paused");
  }
  return heap_;
}

void Debugger::Run() { StartExecution(ExecutionCommand::kRun); }

void Debugger::StepIn() { StartExecution(ExecutionCommand::kStepIn); }

void Debugger::StepOver() { StartExecution(ExecutionCommand::kStepOver); }

void Debugger::StepOut() { StartExecution(ExecutionCommand::kStepOut); }

void Debugger::StartExecution(ExecutionCommand end) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (exec_state_ != ExecutionState::kPaused) {
    common::fail("program is not paused");
  }
  if (exec_thread_.joinable()) {
    exec_thread_.join();
  }
  exec_state_ = ExecutionState::kRunning;
  exec_thread_ = std::thread(&Debugger::Execute, this, end);
}

void Debugger::Pause() {
  std::lock_guard<std::mutex> lock(mutex_);
  switch (exec_state_) {
    case ExecutionState::kPausing:
    case ExecutionState::kPaused:
    case ExecutionState::kTerminated:
      return;
    case ExecutionState::kRunning:
      break;
  }
  exec_state_ = ExecutionState::kPausing;
}

void Debugger::PauseAndAwait() {
  Pause();
  AwaitPause();
}

void Debugger::AwaitPause() {
  std::unique_lock<std::mutex> lock(mutex_);
  cond_.wait(lock, [this] {
    switch (exec_state_) {
      case ExecutionState::kPaused:
      case ExecutionState::kTerminated:
        return true;
      default:
        return false;
    }
  });
  if (exec_thread_.joinable()) {
    exec_thread_.join();
  }
}

void Debugger::AwaitTermination() {
  std::unique_lock<std::mutex> lock(mutex_);
  cond_.wait(lock, [this] { return exec_state_ == ExecutionState::kTerminated; });
  if (exec_thread_.joinable()) {
    exec_thread_.join();
  }
}

void Debugger::Execute(ExecutionCommand command) {
  std::size_t initial_stack_depth = stack_.depth();
  while (true) {
    ExecuteStep();

    if (HasProgramCompleted()) {
      std::lock_guard<std::mutex> lock(mutex_);
      exec_state_ = ExecutionState::kTerminated;
      cond_.notify_all();
      return;

    } else if (ExecutedCommand(command, initial_stack_depth)) {
      std::lock_guard<std::mutex> lock(mutex_);
      exec_state_ = ExecutionState::kPaused;
      cond_.notify_all();
      return;

    } else if (std::lock_guard<std::mutex> lock(mutex_); exec_state_ == ExecutionState::kPausing) {
      exec_state_ = ExecutionState::kPaused;
      cond_.notify_all();
      return;
    }
  }
}

bool Debugger::ExecutedCommand(ExecutionCommand command, std::size_t initial_stack_depth) {
  switch (command) {
    case ExecutionCommand::kStepIn:
      return true;
    case ExecutionCommand::kStepOver:
      return stack_.depth() <= initial_stack_depth;
    case ExecutionCommand::kStepOut:
      return stack_.depth() < initial_stack_depth;
    case ExecutionCommand::kRun:
      return false;
  }
}

}  // namespace ir_interpreter

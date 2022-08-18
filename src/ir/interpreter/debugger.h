//
//  debugger.h
//  Katara
//
//  Created by Arne Philipeit on 8/14/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include <condition_variable>
#include <mutex>
#include <thread>

#include "src/ir/interpreter/execution_point.h"
#include "src/ir/interpreter/heap.h"
#include "src/ir/interpreter/interpreter.h"
#include "src/ir/interpreter/stack.h"

#ifndef ir_interpreter_debugger_h
#define ir_interpreter_debugger_h

namespace ir_interpreter {

class Debugger final : public Interpreter {
 public:
  enum class ExecutionState {
    kRunning,
    kPausing,
    kPaused,
    kTerminated,
  };

  Debugger(ir::Program* program, bool sanitize) : Interpreter(program, sanitize) {}
  ~Debugger() override { PauseAndAwait(); }

  ExecutionState execution_state() const;

  int64_t exit_code() const override;
  const Stack& stack() const;
  const Heap& heap() const;

  void Run() override;
  void StepIn();
  void StepOver();
  void StepOut();

  void Pause();
  void PauseAndAwait();

  void AwaitPause();
  void AwaitTermination();

 private:
  enum class ExecutionCommand {
    kRun,
    kStepIn,
    kStepOver,
    kStepOut,
  };

  void StartExecution(ExecutionCommand command);

  void Execute(ExecutionCommand command);
  bool ExecutedCommand(ExecutionCommand command, std::size_t initial_stack_depth);

  mutable std::mutex mutex_;
  std::condition_variable cond_;
  std::thread exec_thread_;
  ExecutionState exec_state_ = ExecutionState::kPaused;
};

}  // namespace ir_interpreter

#endif /* ir_interpreter_debugger_h */

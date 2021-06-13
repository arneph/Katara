//
//  interpreter.h
//  Katara-tests
//
//  Created by Arne Philipeit on 6/3/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "ir/interpreter/values.h"
#include "ir/representation/block.h"
#include "ir/representation/func.h"
#include "ir/representation/instrs.h"
#include "ir/representation/num_types.h"
#include "ir/representation/program.h"
#include "ir/representation/types.h"
#include "ir/representation/values.h"

#ifndef ir_interpreter_interpreter_h
#define ir_interpreter_interpreter_h

namespace ir_interpreter {

class Interpreter {
 public:
  enum class ExecutionState {
    kReady,
    kRunning,
    kPaused,
    kTerminated,
  };

  Interpreter(ir::Program* program) : state_(ExecutionState::kReady), program_(program) {}

  ExecutionState state() const { return state_; }
  void run();
  void pause();
  
  int exit_code() const { return exit_code_; }

 private:
  struct FuncContext {
    std::unordered_map<ir::value_num_t, Value> computed_values_;
  };

  std::vector<Value> CallFunc(ir::Func* func, std::vector<Value> args);

  void ExecuteBinaryALInstr(ir::BinaryALInstr* instr, FuncContext& ctx);
  Value ComputBinaryOp(Value a, ir::BinaryALOperation op, Value b);
  
  std::vector<Value> Evaluate(const std::vector<std::shared_ptr<ir::Value>>& ir_values,
                              FuncContext& ctx);
  Value Evaluate(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx);

  ExecutionState state_;
  int exit_code_;

  ir::Program* program_;
};

}  // namespace ir_interpreter

#endif /* ir_interpreter_interpreter_h */

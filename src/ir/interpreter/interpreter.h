//
//  interpreter.h
//  Katara-tests
//
//  Created by Arne Philipeit on 6/3/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include <memory>
#include <vector>

#include "src/common/atomics.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"

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

  int64_t exit_code() const { return exit_code_; }

 private:
  struct FuncContext {
    std::unordered_map<ir::value_num_t, std::unique_ptr<ir::Constant>> computed_values_;
  };

  std::vector<std::unique_ptr<ir::Constant>> CallFunc(ir::Func* func,
                                                      std::vector<ir::Constant*> args);

  void ExecuteConversion(ir::Conversion* instr, FuncContext& ctx);
  std::unique_ptr<ir::Constant> ComputeConversion(const ir::Type* result_type,
                                                  ir::Constant* operand);

  void ExecuteIntBinaryInstr(ir::IntBinaryInstr* instr, FuncContext& ctx);
  void ExecuteIntShiftInstr(ir::IntShiftInstr* instr, FuncContext& ctx);

  std::vector<std::unique_ptr<ir::Constant>> EvaluateFuncResults(
      const std::vector<std::shared_ptr<ir::Value>>& ir_values, FuncContext& ctx);

  std::vector<ir::Constant*> Evaluate(const std::vector<std::shared_ptr<ir::Value>>& ir_values,
                                      FuncContext& ctx);

  bool EvaluateBool(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx);
  common::Int EvaluateInt(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx);
  int64_t EvaluatePointer(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx);
  ir::func_num_t EvaluateFunc(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx);
  ir::Constant* Evaluate(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx);

  ExecutionState state_;
  int64_t exit_code_;

  ir::Program* program_;
};

}  // namespace ir_interpreter

#endif /* ir_interpreter_interpreter_h */

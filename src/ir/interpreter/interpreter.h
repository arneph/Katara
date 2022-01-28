//
//  interpreter.h
//  Katara-tests
//
//  Created by Arne Philipeit on 6/3/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include <memory>
#include <vector>

#include "src/common/atomics/atomics.h"
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

  Interpreter(ir::Program* program) : program_(program) {}

  ExecutionState state() const { return state_; }
  void run();
  void pause();

  int64_t exit_code() const { return exit_code_; }

 private:
  typedef std::shared_ptr<ir::Constant> RuntimeConstant;

  struct FuncContext {
    std::unordered_map<ir::value_num_t, RuntimeConstant> computed_values_;
  };

  std::vector<std::shared_ptr<ir::Constant>> CallFunc(ir::Func* func,
                                                      std::vector<RuntimeConstant> args);

  void ExecuteConversion(ir::Conversion* instr, FuncContext& ctx);
  void ExecuteIntBinaryInstr(ir::IntBinaryInstr* instr, FuncContext& ctx);
  void ExecuteIntCompareInstr(ir::IntCompareInstr* instr, FuncContext& ctx);
  void ExecuteIntShiftInstr(ir::IntShiftInstr* instr, FuncContext& ctx);

  void ExecutePointerOffsetInstr(ir::PointerOffsetInstr* instr, FuncContext& ctx);
  void ExecuteNilTestInstr(ir::NilTestInstr* instr, FuncContext& ctx);

  void ExecuteMallocInstr(ir::MallocInstr* instr, FuncContext& ctx);
  void ExecuteLoadInstr(ir::LoadInstr* instr, FuncContext& ctx);
  void ExecuteStoreInstr(ir::StoreInstr* instr, FuncContext& ctx);
  void ExecuteFreeInstr(ir::FreeInstr* instr, FuncContext& ctx);

  void ExecuteCallInstr(ir::CallInstr* instr, FuncContext& ctx);

  bool EvaluateBool(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx);
  common::Int EvaluateInt(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx);
  int64_t EvaluatePointer(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx);
  ir::func_num_t EvaluateFunc(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx);

  std::vector<RuntimeConstant> Evaluate(const std::vector<std::shared_ptr<ir::Value>>& ir_values,
                                        FuncContext& ctx);
  RuntimeConstant Evaluate(std::shared_ptr<ir::Value> ir_value, FuncContext& ctx);

  ExecutionState state_ = ExecutionState::kReady;
  int64_t exit_code_ = 0;

  ir::Program* program_;
};

}  // namespace ir_interpreter

#endif /* ir_interpreter_interpreter_h */

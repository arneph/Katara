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
#include "src/ir/interpreter/execution_point.h"
#include "src/ir/interpreter/heap.h"
#include "src/ir/interpreter/stack.h"
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
  Interpreter(ir::Program* program, bool sanitize);
  virtual ~Interpreter() = default;

  virtual int64_t exit_code() const;

  virtual void Run();

 protected:
  bool HasProgramCompleted() const { return exit_code_.has_value(); }
  void ExecuteStep();

  std::optional<int64_t> exit_code_;
  Stack stack_;
  Heap heap_;

 private:
  std::vector<std::shared_ptr<ir::Constant>> CallFunc(
      ir::Func* func, std::vector<std::shared_ptr<ir::Constant>> args);

  void ExecuteFuncExit();

  void ExecuteInstr(ir::Instr* instr);
  void ExecuteMovInstr(ir::MovInstr* instr);
  void ExecutePhiInstr(ir::PhiInstr* instr);
  void ExecuteConversion(ir::Conversion* instr);
  void ExecuteIntBinaryInstr(ir::IntBinaryInstr* instr);
  void ExecuteIntCompareInstr(ir::IntCompareInstr* instr);
  void ExecuteIntShiftInstr(ir::IntShiftInstr* instr);

  void ExecutePointerOffsetInstr(ir::PointerOffsetInstr* instr);
  void ExecuteNilTestInstr(ir::NilTestInstr* instr);

  void ExecuteMallocInstr(ir::MallocInstr* instr);
  void ExecuteLoadInstr(ir::LoadInstr* instr);
  void ExecuteStoreInstr(ir::StoreInstr* instr);
  void ExecuteFreeInstr(ir::FreeInstr* instr);

  void ExecuteJumpInstr(ir::JumpInstr* instr);
  void ExecuteJumpCondInstr(ir::JumpCondInstr* instr);
  void ExecuteCallInstr(ir::CallInstr* instr);
  void ExecuteReturnInstr(ir::ReturnInstr* instr);

  bool EvaluateBool(std::shared_ptr<ir::Value> ir_value);
  common::Int EvaluateInt(std::shared_ptr<ir::Value> ir_value);
  int64_t EvaluatePointer(std::shared_ptr<ir::Value> ir_value);
  ir::func_num_t EvaluateFunc(std::shared_ptr<ir::Value> ir_value);

  std::vector<std::shared_ptr<ir::Constant>> Evaluate(
      const std::vector<std::shared_ptr<ir::Value>>& ir_values);
  std::shared_ptr<ir::Constant> Evaluate(std::shared_ptr<ir::Value> ir_value);

  StackFrame& current_stack_frame();

  ir::Program* program_;
};

}  // namespace ir_interpreter

#endif /* ir_interpreter_interpreter_h */

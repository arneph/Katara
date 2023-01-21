//
//  checker.h
//  Katara
//
//  Created by Arne Philipeit on 2/20/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_checker_checker_h
#define ir_checker_checker_h

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "src/common/positions/positions.h"
#include "src/ir/issues/issues.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/object.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"

namespace ir_check {

class Checker {
 public:
  Checker(ir_issues::IssueTracker& issue_tracker, const ir::Program* program)
      : issue_tracker_(issue_tracker), program_(program) {}
  virtual ~Checker() = default;

  virtual void CheckProgram();

 protected:
  const ir::Program* program() const { return program_; }
  ir_issues::IssueTracker& issue_tracker() { return issue_tracker_; }

  virtual void CheckFunc(const ir::Func* func);
  virtual void CheckBlock(const ir::Block* block, const ir::Func* func);
  virtual void CheckInstr(const ir::Instr* instr, const ir::Block* block, const ir::Func* func);
  virtual void CheckMovInstr(const ir::MovInstr* mov_instr);
  virtual void CheckPhiInstr(const ir::PhiInstr* phi_instr, const ir::Block* block,
                             const ir::Func* func);
  virtual void CheckLoadInstr(const ir::LoadInstr* load_instr);
  virtual void CheckStoreInstr(const ir::StoreInstr* store_instr);
  virtual void CheckValue(const ir::Computed* value);

 private:
  struct FuncValueReference {
    const ir::Block* block;
    const ir::Instr* instr;
    std::size_t instr_index;
  };
  struct FuncValues {
    std::unordered_map<ir::value_num_t, const ir::Computed*> pointers;
    std::unordered_set<ir::value_num_t> args;
    std::unordered_map<ir::value_num_t, FuncValueReference> definitions;
  };

  void AddValueInFunc(const ir::Computed* value, const ir::Func* func, FuncValues& func_values);
  void AddArgsInFunc(const ir::Func* func, FuncValues& func_values);
  void AddDefinitionsInFunc(const ir::Func* func, FuncValues& func_values);
  void CheckDefinitionDominatesUse(const FuncValueReference& definition,
                                   const FuncValueReference& use, const ir::Func* func);
  void CheckDefinitionDominatesUseInPhi(const FuncValueReference& definition,
                                        const FuncValueReference& use,
                                        const ir::InheritedValue* inherited_value,
                                        const ir::Func* func);
  void CheckValuesInFunc(const ir::Func* func);

  void CheckConversion(const ir::Conversion* conversion);
  void CheckBoolNotInstr(const ir::BoolNotInstr* bool_not_instr);
  void CheckBoolBinaryInstr(const ir::BoolBinaryInstr* bool_binary_instr);
  void CheckIntUnaryInstr(const ir::IntUnaryInstr* int_unary_instr);
  void CheckIntCompareInstr(const ir::IntCompareInstr* int_compare_instr);
  void CheckIntBinaryInstr(const ir::IntBinaryInstr* int_binary_instr);
  void CheckIntShiftInstr(const ir::IntShiftInstr* int_shift_instr);
  void CheckPointerOffsetInstr(const ir::PointerOffsetInstr* pointer_offset_instr);
  void CheckNilTestInstr(const ir::NilTestInstr* nil_test_instr);
  void CheckMallocInstr(const ir::MallocInstr* malloc_instr);
  void CheckFreeInstr(const ir::FreeInstr* free_instr);
  void CheckJumpInstr(const ir::JumpInstr* jump_instr, const ir::Block* block);
  void CheckJumpCondInstr(const ir::JumpCondInstr* jump_cond_instr, const ir::Block* block);
  void CheckSyscallInstr(const ir::SyscallInstr* syscall_instr);
  void CheckCallInstr(const ir::CallInstr* call_instr);
  void CheckReturnInstr(const ir::ReturnInstr* return_instr, const ir::Block* block,
                        const ir::Func* func);

  ir_issues::IssueTracker& issue_tracker_;
  const ir::Program* program_;
  std::unordered_map<const ir::Computed*, const ir::Func*> values_to_funcs_;
};

}  // namespace ir_check

#endif /* ir_checker_checker_h */

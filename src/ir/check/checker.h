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
#include "src/ir/serialization/positions.h"

namespace ir_check {

class Checker {
 public:
  Checker(ir_issues::IssueTracker& issue_tracker, const ir::Program* program,
          const ir_serialization::ProgramPositions& program_positions)
      : issue_tracker_(issue_tracker), program_(program), program_positions_(program_positions) {}
  virtual ~Checker() = default;

  virtual void CheckProgram();

 protected:
  const ir::Program* program() const { return program_; }
  ir_issues::IssueTracker& issue_tracker() { return issue_tracker_; }

  virtual void CheckFunc(const ir::Func* func,
                         const ir_serialization::FuncPositions& func_positions);
  virtual void CheckBlock(const ir::Block* block,
                          const ir_serialization::BlockPositions& block_positions,
                          const ir::Func* func,
                          const ir_serialization::FuncPositions& func_positions);
  virtual void CheckInstr(const ir::Instr* instr,
                          const ir_serialization::InstrPositions& instr_positions,
                          const ir::Block* block, const ir::Func* func,
                          const ir_serialization::FuncPositions& func_positions);
  virtual void CheckMovInstr(const ir::MovInstr* mov_instr,
                             const ir_serialization::InstrPositions& mov_instr_positions);
  virtual void CheckPhiInstr(const ir::PhiInstr* phi_instr,
                             const ir_serialization::InstrPositions& phi_instr_positions,
                             const ir::Block* block, const ir::Func* func);
  virtual void CheckLoadInstr(const ir::LoadInstr* load_instr,
                              const ir_serialization::InstrPositions& load_instr_positions);
  virtual void CheckStoreInstr(const ir::StoreInstr* store_instr,
                               const ir_serialization::InstrPositions& store_instr_positions);
  virtual void CheckValue(const ir::Computed* value, common::positions::range_t value_range);

 private:
  struct FuncValueReference {
    const ir::Block* block;
    const ir::Instr* instr;
    std::size_t instr_index;
    common::positions::range_t range;
  };
  struct FuncValues {
    std::unordered_map<ir::value_num_t, const ir::Computed*> pointers;
    std::unordered_map<ir::value_num_t, common::positions::range_t> args;
    std::unordered_map<ir::value_num_t, FuncValueReference> definitions;
  };

  void AddDefinitionInFunc(const ir::Computed* value, common::positions::range_t value_range,
                           const ir::Func* func, FuncValues& func_values);
  void AddArgsInFunc(const ir::Func* func, const ir_serialization::FuncPositions& func_positions,
                     FuncValues& func_values);
  void AddDefinitionsInFunc(const ir::Func* func, FuncValues& func_values);
  void CheckDefinitionDominatesUse(const FuncValueReference& definition,
                                   const FuncValueReference& use, const ir::Func* func);
  void CheckDefinitionDominatesUseInPhi(const FuncValueReference& definition,
                                        const FuncValueReference& use,
                                        const ir::InheritedValue* inherited_value,
                                        const ir::Func* func);
  void CheckValuesInFunc(const ir::Func* func,
                         const ir_serialization::FuncPositions& func_positions);

  void CheckConversion(const ir::Conversion* conversion,
                       const ir_serialization::InstrPositions& conversion_positions);
  void CheckBoolNotInstr(const ir::BoolNotInstr* bool_not_instr,
                         const ir_serialization::InstrPositions& bool_not_instr_positions);
  void CheckBoolBinaryInstr(const ir::BoolBinaryInstr* bool_binary_instr,
                            const ir_serialization::InstrPositions& bool_binary_instr_positions);
  void CheckIntUnaryInstr(const ir::IntUnaryInstr* int_unary_instr,
                          const ir_serialization::InstrPositions& int_unary_instr_positions);
  void CheckIntCompareInstr(const ir::IntCompareInstr* int_compare_instr,
                            const ir_serialization::InstrPositions& int_compare_instr_positions);
  void CheckIntBinaryInstr(const ir::IntBinaryInstr* int_binary_instr,
                           const ir_serialization::InstrPositions& int_binary_instr_positions);
  void CheckIntShiftInstr(const ir::IntShiftInstr* int_shift_instr,
                          const ir_serialization::InstrPositions& int_shift_instr_positions);
  void CheckPointerOffsetInstr(
      const ir::PointerOffsetInstr* pointer_offset_instr,
      const ir_serialization::InstrPositions& pointer_offset_instr_positions);
  void CheckNilTestInstr(const ir::NilTestInstr* nil_test_instr,
                         const ir_serialization::InstrPositions& nil_test_instr_positions);
  void CheckMallocInstr(const ir::MallocInstr* malloc_instr,
                        const ir_serialization::InstrPositions& malloc_instr_positions);
  void CheckFreeInstr(const ir::FreeInstr* free_instr,
                      const ir_serialization::InstrPositions& free_instr_positions);
  void CheckJumpInstr(const ir::JumpInstr* jump_instr,
                      const ir_serialization::InstrPositions& jump_instr_positions,
                      const ir::Block* block);
  void CheckJumpCondInstr(const ir::JumpCondInstr* jump_cond_instr,
                          const ir_serialization::InstrPositions& jump_cond_instr_positions,
                          const ir::Block* block);
  void CheckSyscallInstr(const ir::SyscallInstr* syscall_instr,
                         const ir_serialization::InstrPositions& syscall_instr_positions);
  void CheckCallInstr(const ir::CallInstr* call_instr,
                      const ir_serialization::InstrPositions& call_instr_positions);
  void CheckReturnInstr(const ir::ReturnInstr* return_instr,
                        const ir_serialization::InstrPositions& return_instr_positions,
                        const ir::Block* block, const ir::Func* func,
                        const ir_serialization::FuncPositions& func_positions);

  ir_issues::IssueTracker& issue_tracker_;
  const ir::Program* program_;
  const ir_serialization::ProgramPositions& program_positions_;
  std::unordered_map<const ir::Computed*, const ir::Func*> values_to_funcs_;
  std::unordered_map<const ir::Computed*, common::positions::range_t> values_to_ranges_;
};

}  // namespace ir_check

#endif /* ir_checker_checker_h */

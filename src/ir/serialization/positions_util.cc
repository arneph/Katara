//
//  positions_util.cc
//  Katara
//
//  Created by Arne Philipeit on 10/7/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#include "positions_util.h"

#include "src/ir/representation/instrs.h"

namespace ir_serialization {

using ::common::positions::range_t;

range_t GetMovInstrResultRange(const InstrPositions& mov_instr_positions) {
  return mov_instr_positions.defined_value_ranges().at(ir::MovInstr::kResultIndex);
}

range_t GetMovInstrOperandRange(const InstrPositions& mov_instr_positions) {
  return mov_instr_positions.used_value_ranges().at(ir::MovInstr::kOperandIndex);
}

range_t GetPhiInstrResultRange(const InstrPositions& phi_instr_positions) {
  return phi_instr_positions.defined_value_ranges().at(ir::PhiInstr::kResultIndex);
}

range_t GetPhiInstrOperandRange(std::size_t operand_index,
                                const InstrPositions& phi_instr_positions) {
  return phi_instr_positions.used_value_ranges().at(operand_index);
}

range_t GetConversionResultRange(const InstrPositions& conversion_positions) {
  return conversion_positions.defined_value_ranges().at(ir::Conversion::kResultIndex);
}

range_t GetConversionOperandRange(const InstrPositions& conversion_positions) {
  return conversion_positions.used_value_ranges().at(ir::Conversion::kOperandIndex);
}

range_t GetBoolNotInstrResultRange(const InstrPositions& bool_not_instr_positions) {
  return bool_not_instr_positions.defined_value_ranges().at(ir::BoolNotInstr::kResultIndex);
}

range_t GetBoolNotInstrOperandRange(const InstrPositions& bool_not_instr_positions) {
  return bool_not_instr_positions.used_value_ranges().at(ir::BoolNotInstr::kOperandIndex);
}

range_t GetBoolBinaryInstrResultRange(const InstrPositions& bool_binary_instr_positions) {
  return bool_binary_instr_positions.defined_value_ranges().at(ir::BoolBinaryInstr::kResultIndex);
}

range_t GetBoolBinaryInstrOperandARange(const InstrPositions& bool_binary_instr_positions) {
  return bool_binary_instr_positions.used_value_ranges().at(ir::BoolBinaryInstr::kOperandAIndex);
}

range_t GetBoolBinaryInstrOperandBRange(const InstrPositions& bool_binary_instr_positions) {
  return bool_binary_instr_positions.used_value_ranges().at(ir::BoolBinaryInstr::kOperandBIndex);
}

range_t GetIntUnaryInstrResultRange(const InstrPositions& int_unary_instr_positions) {
  return int_unary_instr_positions.defined_value_ranges().at(ir::IntUnaryInstr::kResultIndex);
}

range_t GetIntUnaryInstrOperandRange(const InstrPositions& int_unary_instr_positions) {
  return int_unary_instr_positions.used_value_ranges().at(ir::IntUnaryInstr::kOperandIndex);
}

range_t GetIntCompareInstrResultRange(const InstrPositions& int_compare_instr_positions) {
  return int_compare_instr_positions.defined_value_ranges().at(ir::IntCompareInstr::kResultIndex);
}

range_t GetIntCompareInstrOperandARange(const InstrPositions& int_compare_instr_positions) {
  return int_compare_instr_positions.used_value_ranges().at(ir::IntCompareInstr::kOperandAIndex);
}

range_t GetIntCompareInstrOperandBRange(const InstrPositions& int_compare_instr_positions) {
  return int_compare_instr_positions.used_value_ranges().at(ir::IntCompareInstr::kOperandBIndex);
}

range_t GetIntBinaryInstrResultRange(const InstrPositions& int_binary_instr_positions) {
  return int_binary_instr_positions.defined_value_ranges().at(ir::IntBinaryInstr::kResultIndex);
}

range_t GetIntBinaryInstrOperandARange(const InstrPositions& int_binary_instr_positions) {
  return int_binary_instr_positions.used_value_ranges().at(ir::IntBinaryInstr::kOperandAIndex);
}

range_t GetIntBinaryInstrOperandBRange(const InstrPositions& int_binary_instr_positions) {
  return int_binary_instr_positions.used_value_ranges().at(ir::IntBinaryInstr::kOperandBIndex);
}

range_t GetIntShiftInstrResultRange(const InstrPositions& int_shift_instr_positions) {
  return int_shift_instr_positions.defined_value_ranges().at(ir::IntShiftInstr::kResultIndex);
}

range_t GetIntShiftInstrShiftedRange(const InstrPositions& int_shift_instr_positions) {
  return int_shift_instr_positions.used_value_ranges().at(ir::IntShiftInstr::kShiftedIndex);
}

range_t GetIntShiftInstrOffsetRange(const InstrPositions& int_shift_instr_positions) {
  return int_shift_instr_positions.used_value_ranges().at(ir::IntShiftInstr::kOffsetIndex);
}

range_t GetPointerOffsetInstrResultRange(const InstrPositions& pointer_offset_instr_positions) {
  return pointer_offset_instr_positions.defined_value_ranges().at(
      ir::PointerOffsetInstr::kResultIndex);
}

range_t GetPointerOffsetInstrPointerRange(const InstrPositions& pointer_offset_instr_positions) {
  return pointer_offset_instr_positions.used_value_ranges().at(
      ir::PointerOffsetInstr::kPointerIndex);
}

range_t GetPointerOffsetInstrOffsetRange(const InstrPositions& pointer_offset_instr_positions) {
  return pointer_offset_instr_positions.used_value_ranges().at(
      ir::PointerOffsetInstr::kOffsetIndex);
}

range_t GetNilTestInstrResultRange(const InstrPositions& nil_test_instr_positions) {
  return nil_test_instr_positions.defined_value_ranges().at(ir::NilTestInstr::kResultIndex);
}

range_t GetNilTestInstrTestedRange(const InstrPositions& nil_test_instr_positions) {
  return nil_test_instr_positions.used_value_ranges().at(ir::NilTestInstr::kTestedIndex);
}

range_t GetMallocInstrResultRange(const InstrPositions& malloc_instr_positions) {
  return malloc_instr_positions.defined_value_ranges().at(ir::MallocInstr::kResultIndex);
}

range_t GetMallocInstrSizeRange(const InstrPositions& malloc_instr_positions) {
  return malloc_instr_positions.used_value_ranges().at(ir::MallocInstr::kSizeIndex);
}

range_t GetLoadInstrResultRange(const InstrPositions& load_instr_positions) {
  return load_instr_positions.defined_value_ranges().at(ir::LoadInstr::kResultIndex);
}

range_t GetLoadInstrAddressRange(const InstrPositions& load_instr_positions) {
  return load_instr_positions.used_value_ranges().at(ir::LoadInstr::kAddressIndex);
}

range_t GetStoreInstrAddressRange(const InstrPositions& store_instr_positions) {
  return store_instr_positions.used_value_ranges().at(ir::StoreInstr::kAddressIndex);
}

range_t GetStoreInstrValueRange(const InstrPositions& store_instr_positions) {
  return store_instr_positions.used_value_ranges().at(ir::StoreInstr::kValueIndex);
}

range_t GetFreeInstrAddressRange(const InstrPositions& free_instr_positions) {
  return free_instr_positions.used_value_ranges().at(ir::FreeInstr::kAddressIndex);
}

range_t GetJumpInstrDestinationRange(const InstrPositions& jump_instr_positions) {
  return jump_instr_positions.used_value_ranges().at(ir::JumpInstr::kDestinationIndex);
}

range_t GetJumpCondInstrDestinationTrueRange(const InstrPositions& jump_cond_instr_positions) {
  return jump_cond_instr_positions.used_value_ranges().at(ir::JumpCondInstr::kDestinationTrueIndex);
}

range_t GetJumpCondInstrDestinationFalseRange(const InstrPositions& jump_cond_instr_positions) {
  return jump_cond_instr_positions.used_value_ranges().at(
      ir::JumpCondInstr::kDestinationFalseIndex);
}

range_t GetSyscallInstrResultRange(const InstrPositions& syscall_instr_positions) {
  return syscall_instr_positions.defined_value_ranges().at(ir::SyscallInstr::kResultIndex);
}

range_t GetSyscallInstrNumberRange(const InstrPositions& syscall_instr_positions) {
  return syscall_instr_positions.used_value_ranges().at(ir::SyscallInstr::kNumberIndex);
}

range_t GetSyscallInstrArgRange(std::size_t arg_index,
                                const InstrPositions& syscall_instr_positions) {
  return syscall_instr_positions.used_value_ranges().at(arg_index + 1);
}

range_t GetCallInstrResultRange(std::size_t result_index,
                                const InstrPositions& call_instr_positions) {
  return call_instr_positions.defined_value_ranges().at(result_index);
}

range_t GetCallInstrFuncRange(const InstrPositions& call_instr_positions) {
  return call_instr_positions.used_value_ranges().at(ir::CallInstr::kFuncIndex);
}

range_t GetCallInstrArgRange(std::size_t arg_index, const InstrPositions& call_instr_positions) {
  return call_instr_positions.used_value_ranges().at(arg_index + 1);
}

range_t GetReturnInstrResultRange(std::size_t result_index,
                                  const InstrPositions& return_instr_positions) {
  return return_instr_positions.used_value_ranges().at(result_index);
}

}  // namespace ir_serialization

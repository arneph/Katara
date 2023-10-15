//
//  positions_util.cc
//  Katara
//
//  Created by Arne Philipeit on 10/15/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#include "positions_util.h"

#include "src/common/positions/positions.h"
#include "src/ir/serialization/positions.h"
#include "src/lang/representation/ir_extension/instrs.h"

namespace lang {
namespace ir_serialization {

using ::common::positions::range_t;
using ::ir_serialization::InstrPositions;

range_t GetMakeSharedPointerInstrResultRange(
    const InstrPositions& make_shared_pointer_instr_positions) {
  return make_shared_pointer_instr_positions.defined_value_ranges().at(
      ir_ext::MakeSharedPointerInstr::kResultIndex);
}

range_t GetMakeSharedPointerInstrSizeRange(
    const InstrPositions& make_shared_pointer_instr_positions) {
  return make_shared_pointer_instr_positions.used_value_ranges().at(
      ir_ext::MakeSharedPointerInstr::kSizeIndex);
}

range_t GetCopySharedPointerInstrResultRange(
    const InstrPositions& copy_shared_pointer_instr_positions) {
  return copy_shared_pointer_instr_positions.defined_value_ranges().at(
      ir_ext::CopySharedPointerInstr::kResultIndex);
}

range_t GetCopySharedPointerInstrCopiedRange(
    const InstrPositions& copy_shared_pointer_instr_positions) {
  return copy_shared_pointer_instr_positions.used_value_ranges().at(
      ir_ext::CopySharedPointerInstr::kCopiedSharedPointerIndex);
}

range_t GetCopySharedPointerInstrOffsetRange(
    const InstrPositions& copy_shared_pointer_instr_positions) {
  return copy_shared_pointer_instr_positions.used_value_ranges().at(
      ir_ext::CopySharedPointerInstr::kPointerOffsetIndex);
}

range_t GetDeleteSharedPointerInstrDeletedRange(
    const InstrPositions& delete_shared_pointer_instr_positions) {
  return delete_shared_pointer_instr_positions.used_value_ranges().at(
      ir_ext::DeleteSharedPointerInstr::kDeletedSharedPointerIndex);
}

range_t GetMakeUniquePointerInstrResultRange(
    const InstrPositions& make_unique_pointer_instr_positions) {
  return make_unique_pointer_instr_positions.defined_value_ranges().at(
      ir_ext::MakeUniquePointerInstr::kResultIndex);
}

range_t GetMakeUniquePointerInstrSizeRange(
    const InstrPositions& make_unique_pointer_instr_positions) {
  return make_unique_pointer_instr_positions.used_value_ranges().at(
      ir_ext::MakeUniquePointerInstr::kResultIndex);
}

range_t GetDeleteUniquePointerInstrDeletedRange(
    const InstrPositions& delete_unique_pointer_instr_positions) {
  return delete_unique_pointer_instr_positions.used_value_ranges().at(
      ir_ext::DeleteUniquePointerInstr::kDeletedUniquePointerIndex);
}

range_t GetStringIndexInstrResultRange(const InstrPositions& string_index_instr_positions) {
  return string_index_instr_positions.defined_value_ranges().at(
      ir_ext::StringIndexInstr::kResultIndex);
}

range_t GetStringIndexInstrStringOperandRange(const InstrPositions& string_index_instr_positions) {
  return string_index_instr_positions.used_value_ranges().at(
      ir_ext::StringIndexInstr::kStringOperandIndex);
}

range_t GetStringIndexInstrIndexOperandRange(const InstrPositions& string_index_instr_positions) {
  return string_index_instr_positions.used_value_ranges().at(
      ir_ext::StringIndexInstr::kIndexOperandIndex);
}

range_t GetStringConcatInstrResultRange(const InstrPositions& string_concat_instr_positions) {
  return string_concat_instr_positions.defined_value_ranges().at(
      ir_ext::StringConcatInstr::kResultIndex);
}

range_t GetStringConcatInstrOperandRange(std::size_t operand_index,
                                         const InstrPositions& string_concat_instr_positions) {
  return string_concat_instr_positions.used_value_ranges().at(operand_index);
}

}  // namespace ir_serialization
}  // namespace lang

//
//  positions_util.h
//  Katara
//
//  Created by Arne Philipeit on 10/15/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_serialization_positions_util_h
#define lang_ir_serialization_positions_util_h

#include <cstddef>

#include "src/common/positions/positions.h"
#include "src/ir/serialization/positions.h"

namespace lang {
namespace ir_serialization {

common::positions::range_t GetMakeSharedPointerInstrResultRange(
    const ::ir_serialization::InstrPositions& make_shared_pointer_instr_positions);
common::positions::range_t GetMakeSharedPointerInstrSizeRange(
    const ::ir_serialization::InstrPositions& make_shared_pointer_instr_positions);

common::positions::range_t GetCopySharedPointerInstrResultRange(
    const ::ir_serialization::InstrPositions& copy_shared_pointer_instr_positions);
common::positions::range_t GetCopySharedPointerInstrCopiedRange(
    const ::ir_serialization::InstrPositions& copy_shared_pointer_instr_positions);
common::positions::range_t GetCopySharedPointerInstrOffsetRange(
    const ::ir_serialization::InstrPositions& copy_shared_pointer_instr_positions);

common::positions::range_t GetDeleteSharedPointerInstrDeletedRange(
    const ::ir_serialization::InstrPositions& delete_shared_pointer_instr_positions);

common::positions::range_t GetMakeUniquePointerInstrResultRange(
    const ::ir_serialization::InstrPositions& make_unique_pointer_instr_positions);
common::positions::range_t GetMakeUniquePointerInstrSizeRange(
    const ::ir_serialization::InstrPositions& make_unique_pointer_instr_positions);

common::positions::range_t GetDeleteUniquePointerInstrDeletedRange(
    const ::ir_serialization::InstrPositions& delete_unique_pointer_instr_positions);

common::positions::range_t GetStringIndexInstrResultRange(
    const ::ir_serialization::InstrPositions& string_index_instr_positions);
common::positions::range_t GetStringIndexInstrStringOperandRange(
    const ::ir_serialization::InstrPositions& string_index_instr_positions);
common::positions::range_t GetStringIndexInstrIndexOperandRange(
    const ::ir_serialization::InstrPositions& string_index_instr_positions);

common::positions::range_t GetStringConcatInstrResultRange(
    const ::ir_serialization::InstrPositions& string_concat_instr_positions);
common::positions::range_t GetStringConcatInstrOperandRange(
    std::size_t operand_index,
    const ::ir_serialization::InstrPositions& string_concat_instr_positions);

}  // namespace ir_serialization
}  // namespace lang

#endif /* lang_ir_serialization_positions_util_h */

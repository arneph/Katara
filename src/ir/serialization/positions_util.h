//
//  positions_util.h
//  Katara
//
//  Created by Arne Philipeit on 10/7/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_positions_util_h
#define ir_serialization_positions_util_h

#include <cstddef>

#include "src/common/positions/positions.h"
#include "src/ir/serialization/positions.h"

namespace ir_serialization {

common::positions::range_t GetMovInstrResultRange(const InstrPositions& mov_instr_positions);
common::positions::range_t GetMovInstrOperandRange(const InstrPositions& mov_instr_positions);

common::positions::range_t GetPhiInstrResultRange(const InstrPositions& phi_instr_positions);
common::positions::range_t GetPhiInstrOperandRange(std::size_t operand_index,
                                                   const InstrPositions& phi_instr_positions);

common::positions::range_t GetConversionResultRange(const InstrPositions& conversion_positions);
common::positions::range_t GetConversionOperandRange(const InstrPositions& conversion_positions);

common::positions::range_t GetBoolNotInstrResultRange(
    const InstrPositions& bool_not_instr_positions);
common::positions::range_t GetBoolNotInstrOperandRange(
    const InstrPositions& bool_not_instr_positions);

common::positions::range_t GetBoolBinaryInstrResultRange(
    const InstrPositions& bool_binary_instr_positions);
common::positions::range_t GetBoolBinaryInstrOperandARange(
    const InstrPositions& bool_binary_instr_positions);
common::positions::range_t GetBoolBinaryInstrOperandBRange(
    const InstrPositions& bool_binary_instr_positions);

common::positions::range_t GetIntUnaryInstrResultRange(
    const InstrPositions& int_unary_instr_positions);
common::positions::range_t GetIntUnaryInstrOperandRange(
    const InstrPositions& int_unary_instr_positions);

common::positions::range_t GetIntCompareInstrResultRange(
    const InstrPositions& int_compare_instr_positions);
common::positions::range_t GetIntCompareInstrOperandARange(
    const InstrPositions& int_compare_instr_positions);
common::positions::range_t GetIntCompareInstrOperandBRange(
    const InstrPositions& int_compare_instr_positions);

common::positions::range_t GetIntBinaryInstrResultRange(
    const InstrPositions& int_binary_instr_positions);
common::positions::range_t GetIntBinaryInstrOperandARange(
    const InstrPositions& int_binary_instr_positions);
common::positions::range_t GetIntBinaryInstrOperandBRange(
    const InstrPositions& int_binary_instr_positions);

common::positions::range_t GetIntShiftInstrResultRange(
    const InstrPositions& int_shift_instr_positions);
common::positions::range_t GetIntShiftInstrShiftedRange(
    const InstrPositions& int_shift_instr_positions);
common::positions::range_t GetIntShiftInstrOffsetRange(
    const InstrPositions& int_shift_instr_positions);

common::positions::range_t GetPointerOffsetInstrResultRange(
    const InstrPositions& pointer_offset_instr_positions);
common::positions::range_t GetPointerOffsetInstrPointerRange(
    const InstrPositions& pointer_offset_instr_positions);
common::positions::range_t GetPointerOffsetInstrOffsetRange(
    const InstrPositions& pointer_offset_instr_positions);

common::positions::range_t GetNilTestInstrResultRange(
    const InstrPositions& nil_test_instr_positions);
common::positions::range_t GetNilTestInstrTestedRange(
    const InstrPositions& nil_test_instr_positions);

common::positions::range_t GetMallocInstrResultRange(const InstrPositions& malloc_instr_positions);
common::positions::range_t GetMallocInstrSizeRange(const InstrPositions& malloc_instr_positions);

common::positions::range_t GetLoadInstrResultRange(const InstrPositions& load_instr_positions);
common::positions::range_t GetLoadInstrAddressRange(const InstrPositions& load_instr_positions);

common::positions::range_t GetStoreInstrAddressRange(const InstrPositions& store_instr_positions);
common::positions::range_t GetStoreInstrValueRange(const InstrPositions& store_instr_positions);

common::positions::range_t GetFreeInstrAddressRange(const InstrPositions& free_instr_positions);

common::positions::range_t GetJumpInstrDestinationRange(const InstrPositions& jump_instr_positions);

common::positions::range_t GetJumpCondInstrConditionRange(
    const InstrPositions& jump_cond_instr_positions);
common::positions::range_t GetJumpCondInstrDestinationTrueRange(
    const InstrPositions& jump_cond_instr_positions);
common::positions::range_t GetJumpCondInstrDestinationFalseRange(
    const InstrPositions& jump_cond_instr_positions);

common::positions::range_t GetSyscallInstrResultRange(
    const InstrPositions& syscall_instr_positions);
common::positions::range_t GetSyscallInstrNumberRange(
    const InstrPositions& syscall_instr_positions);
common::positions::range_t GetSyscallInstrArgRange(std::size_t arg_index,
                                                   const InstrPositions& syscall_instr_positions);

common::positions::range_t GetCallInstrResultRange(std::size_t result_index,
                                                   const InstrPositions& call_instr_positions);
common::positions::range_t GetCallInstrFuncRange(const InstrPositions& call_instr_positions);
common::positions::range_t GetCallInstrArgRange(std::size_t arg_index,
                                                const InstrPositions& call_instr_positions);

common::positions::range_t GetReturnInstrResultRange(std::size_t result_index,
                                                     const InstrPositions& return_instr_positions);

}  // namespace ir_serialization

#endif /* ir_serialization_positions_util_h */

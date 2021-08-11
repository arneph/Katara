//
//  arithmetic_logic_instrs_translator.h
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_arithmetic_logic_instrs_translator_h
#define ir_to_x86_64_translator_arithmetic_logic_instrs_translator_h

#include "src/ir/representation/instrs.h"
#include "src/x86_64/ir_translator/context.h"

namespace ir_to_x86_64_translator {

void TranslateBoolNotInstr(ir::BoolNotInstr* ir_bool_not_instr, BlockContext& ctx);
void TranslateBoolBinaryInstr(ir::BoolBinaryInstr* ir_bool_binary_instr, BlockContext& ctx);
void TranslateBoolCompareInstr(ir::BoolBinaryInstr* ir_bool_compare_instr, BlockContext& ctx);
void TranslateBoolLogicInstr(ir::BoolBinaryInstr* ir_bool_logic_instr, BlockContext& ctx);

void TranslateIntUnaryInstr(ir::IntUnaryInstr* ir_int_unary_instr, BlockContext& ctx);
void TranslateIntCompareInstr(ir::IntCompareInstr* ir_int_compare_instr, BlockContext& ctx);
void TranslateIntBinaryInstr(ir::IntBinaryInstr* ir_int_binary_instr, BlockContext& ctx);
void TranslateIntSimpleALInstr(ir::IntBinaryInstr* ir_int_binary_instr, BlockContext& ctx);
void TranslateIntMulInstr(ir::IntBinaryInstr* ir_int_binary_instr, BlockContext& ctx);
void TranslateIntDivOrRemInstr(ir::IntBinaryInstr* ir_int_binary_instr, BlockContext& ctx);
void TranslateIntShiftInstr(ir::IntShiftInstr* ir_int_shift_instr, BlockContext& ctx);

void TranslatePointerOffsetInstr(ir::PointerOffsetInstr* ir_pointer_offset_instr,
                                 BlockContext& ctx);
void TranslateNilTestInstr(ir::NilTestInstr* ir_nil_test_instr, BlockContext& ctx);

}  // namespace ir_to_x86_64_translator

#endif /* ir_to_x86_64_translator_arithmetic_logic_instrs_translator_h */

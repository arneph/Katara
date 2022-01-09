//
//  value_translator.h
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_value_translator_h
#define ir_to_x86_64_translator_value_translator_h

#include "src/ir/representation/num_types.h"
#include "src/ir/representation/values.h"
#include "src/x86_64/ir_translator/context.h"
#include "src/x86_64/ops.h"

namespace ir_to_x86_64_translator {

enum class IntNarrowing {
  kNone,
  k64To32BitIfPossible,
};

x86_64::Operand TranslateValue(ir::Value* value, IntNarrowing narrowing, FuncContext& ctx);
x86_64::Imm TranslateBoolConstant(ir::BoolConstant* constant);
x86_64::Imm TranslateIntConstant(ir::IntConstant* constant, IntNarrowing narrowing);
x86_64::Imm TranslatePointerConstant(ir::PointerConstant* constant);
x86_64::Operand TranslateFuncConstant(ir::FuncConstant* constant, ProgramContext& ctx);
x86_64::RM TranslateComputed(ir::Computed* computed, FuncContext& ctx);
x86_64::BlockRef TranslateBlockValue(ir::block_num_t block_value, FuncContext& ctx);

}  // namespace ir_to_x86_64_translator

#endif /* ir_to_x86_64_translator_value_translator_h */

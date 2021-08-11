//
//  control_flow_instrs_translator.h
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_control_flow_instrs_translator_h
#define ir_to_x86_64_translator_control_flow_instrs_translator_h

#include "src/ir/representation/instrs.h"
#include "src/x86_64/ir_translator/context.h"

namespace ir_to_x86_64_translator {

void TranslateJumpInstr(ir::JumpInstr* ir_jump_instr, BlockContext& ctx);
void TranslateJumpCondInstr(ir::JumpCondInstr* ir_jump_cond_instr, BlockContext& ctx);

void TranslateCallInstr(ir::CallInstr* ir_call_instr, BlockContext& ctx);
void TranslateReturnInstr(ir::ReturnInstr* ir_return_instr, BlockContext& ctx);

}  // namespace ir_to_x86_64_translator

#endif /* ir_to_x86_64_translator_control_flow_instrs_translator_h */

//
//  data_instrs_translator.h
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_data_instrs_translator_h
#define ir_to_x86_64_translator_data_instrs_translator_h

#include "src/ir/representation/instrs.h"
#include "src/x86_64/ir_translator/context.h"

namespace ir_to_x86_64_translator {

void TranslateMovInstr(ir::MovInstr* ir_mov_instr, BlockContext& ctx);
void TranslateMallocInstr(ir::MallocInstr* ir_malloc_instr, BlockContext& ctx);
void TranslateLoadInstr(ir::LoadInstr* ir_load_instr, BlockContext& ctx);
void TranslateStoreInstr(ir::StoreInstr* ir_store_instr, BlockContext& ctx);
void TranslateFreeInstr(ir::FreeInstr* ir_free_instr, BlockContext& ctx);

}  // namespace ir_to_x86_64_translator

#endif /* ir_to_x86_64_translator_data_instrs_translator_h */

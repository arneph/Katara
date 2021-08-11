//
//  instrs_translator.h
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_instrs_translator_h
#define ir_to_x86_64_translator_instrs_translator_h

#include "src/ir/representation/instrs.h"
#include "src/x86_64/ir_translator/context.h"

namespace ir_to_x86_64_translator {

void TranslateInstr(ir::Instr* ir_instr, BlockContext& ctx);

}

#endif /* ir_to_x86_64_translator_instrs_translator_h */

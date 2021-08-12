//
//  mov_generator.h
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_mov_generator_h
#define ir_to_x86_64_translator_mov_generator_h

#include "src/ir/representation/instrs.h"
#include "src/ir/representation/values.h"
#include "src/x86_64/ir_translator/context.h"

namespace ir_to_x86_64_translator {

void GenerateMov(x86_64::RM x86_64_result, x86_64::Operand x86_64_origin, ir::Instr* instr,
                 BlockContext& ctx);

}

#endif /* ir_to_x86_64_translator_mov_generator_h */

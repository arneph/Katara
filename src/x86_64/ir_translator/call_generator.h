//
//  call_generator.h
//  Katara
//
//  Created by Arne Philipeit on 1/23/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_call_generator_h
#define ir_to_x86_64_translator_call_generator_h

#include <vector>

#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/values.h"
#include "src/x86_64/ir_translator/context.h"
#include "src/x86_64/ops.h"

namespace ir_to_x86_64_translator {

void GenerateCall(ir::Instr* ir_instr, ir::Value* ir_called_func,
                  std::vector<ir::Computed*> ir_results, std::vector<ir::Value*> ir_args,
                  BlockContext& ctx);
void GenerateCall(ir::Instr* ir_instr, x86_64::FuncRef x86_64_called_func,
                  std::vector<ir::Computed*> ir_results, std::vector<ir::Value*> ir_args,
                  BlockContext& ctx);

}  // namespace ir_to_x86_64_translator

#endif /* ir_to_x86_64_translator_call_generator_h */

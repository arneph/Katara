//
//  context.cc
//  Katara
//
//  Created by Arne Philipeit on 8/10/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "context.h"

namespace ir_to_x86_64_translator {

x86_64::func_num_t ProgramContext::x86_64_func_num_for_ir_func_num(
    ir::func_num_t ir_func_num) const {
  return ir_to_x86_64_func_nums_.at(ir_func_num);
}

void ProgramContext::set_x86_64_func_num_for_ir_func_num(ir::func_num_t ir_func_num,
                                                         x86_64::func_num_t x86_64_func_num) {
  ir_to_x86_64_func_nums_.insert_or_assign(ir_func_num, x86_64_func_num);
}

x86_64::block_num_t FuncContext::x86_64_block_num_for_ir_block_num(
    ir::block_num_t ir_block_num) const {
  return ir_to_x86_64_block_nums_.at(ir_block_num);
}

void FuncContext::set_x86_64_block_num_for_ir_block_num(ir::block_num_t ir_block_num,
                                                        x86_64::block_num_t x86_64_block_num) {
  ir_to_x86_64_block_nums_.insert_or_assign(ir_block_num, x86_64_block_num);
}

}  // namespace ir_to_x86_64_translator

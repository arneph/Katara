//
//  mov_generator.h
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_mov_generator_h
#define ir_to_x86_64_translator_mov_generator_h

#include <vector>

#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/instrs.h"
#include "src/x86_64/ir_translator/context.h"
#include "src/x86_64/ir_translator/register_allocator.h"

namespace ir_to_x86_64_translator {

void GenerateMov(x86_64::RM x86_64_result, x86_64::Operand x86_64_origin, ir::Instr* instr,
                 BlockContext& ctx);

class MoveOperation {
 public:
  MoveOperation(x86_64::RM result, x86_64::Operand origin) : result_(result), origin_(origin) {}

  x86_64::RM result() const { return result_; }
  x86_64::Operand origin() const { return origin_; }

  ir_info::color_t result_color() const { return OperandToColor(result_); }
  ir_info::color_t origin_color() const {
    return origin_.is_rm() ? OperandToColor(origin_.rm()) : ir_info::kNoColor;
  }

 private:
  x86_64::RM result_;
  x86_64::Operand origin_;
};

// Generates the mov instructions to complete the given MoveOperations. The set of origins may be
// smaller than the set of results.
void GenerateMovs(std::vector<MoveOperation> operations, ir::Instr* instr, BlockContext& ctx);

}  // namespace ir_to_x86_64_translator

#endif /* ir_to_x86_64_translator_mov_generator_h */

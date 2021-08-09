//
//  register_allocator.h
//  Katara
//
//  Created by Arne Philipeit on 7/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef x86_64_ir_translator_register_allocator_h
#define x86_64_ir_translator_register_allocator_h

#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/func.h"
#include "src/x86_64/ops.h"

namespace x86_64_ir_translator {

enum class RegSavingBehaviour {
  kByCaller,
  kByCallee,
};

x86_64::RM OperandForArg(int arg_index, x86_64::Size size);
x86_64::RM OperandForResult(int result_index, x86_64::Size size);

RegSavingBehaviour SavingBehaviourForReg(x86_64::Reg reg);

x86_64::RM ColorAndSizeToOperand(ir_info::color_t color, x86_64::Size size);
ir_info::color_t OperandToColor(x86_64::RM operand);

const ir_info::InterferenceGraphColors AllocateRegistersInFunc(
    const ir::Func* func, const ir_info::InterferenceGraph& graph);

}  // namespace x86_64_ir_translator

#endif /* register_allocator_hpp */

//
//  register_allocator.h
//  Katara
//
//  Created by Arne Philipeit on 7/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_register_allocator_h
#define ir_to_x86_64_translator_register_allocator_h

#include <unordered_map>

#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/x86_64/ops.h"

namespace ir_to_x86_64_translator {

enum class RegSavingBehaviour {
  kByCaller,
  kByCallee,
};

x86_64::RM OperandForArg(int arg_index, x86_64::Size size);
x86_64::RM OperandForResult(int result_index, x86_64::Size size);

RegSavingBehaviour SavingBehaviourForReg(x86_64::Reg reg);

x86_64::RM ColorAndSizeToOperand(ir_info::color_t color, x86_64::Size size);
ir_info::color_t OperandToColor(x86_64::RM operand);

std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraphColors> AllocateRegisters(
    const ir::Program* program,
    const std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraph>&
        interference_graphs);

}  // namespace ir_to_x86_64_translator

#endif /* ir_to_x86_64_translator_register_allocator_h */

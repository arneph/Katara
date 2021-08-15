//
//  register_allocator.cc
//  Katara
//
//  Created by Arne Philipeit on 7/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "register_allocator.h"

#include "src/common/logging.h"
#include "src/ir/analyzers/interference_graph_colorer.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/values.h"

namespace ir_to_x86_64_translator {

x86_64::RM OperandForArg(int arg_index, x86_64::Size size) {
  switch (arg_index) {
    case 0:
      return x86_64::Reg(size, 7);  // rdi
      return x86_64::rdi;
    case 1:
      return x86_64::Reg(size, 6);  // rsi
      return x86_64::rsi;
    case 2:
      return x86_64::Reg(size, 2);  // rdx
      return x86_64::rdx;
    case 3:
      return x86_64::Reg(size, 1);  // rcx
      return x86_64::rcx;
    case 4:
      return x86_64::Reg(size, 8);  // r8
      return x86_64::r8;
    case 5:
      return x86_64::Reg(size, 9);  // r9
      return x86_64::r9;
    default:
      common::fail("can not handle functions with more than six arguments");
  }
}

x86_64::RM OperandForResult(int result_index, x86_64::Size size) {
  switch (result_index) {
    case 0:
      return x86_64::Reg(size, 0);  // rax
    case 1:
      return x86_64::Reg(size, 2);  // rdx
    default:
      common::fail("can not handle functions with more than two return values");
  }
}

RegSavingBehaviour SavingBehaviourForReg(x86_64::Reg reg) {
  switch (reg.reg()) {
    case 3:   // rbx
    case 4:   // rsp
    case 5:   // rbp
    case 12:  // r12
    case 13:  // r13
    case 14:  // r14
    case 15:  // r15
      return RegSavingBehaviour::kByCallee;
    default:
      return RegSavingBehaviour::kByCaller;
  }
}

x86_64::RM ColorAndSizeToOperand(ir_info::color_t color, x86_64::Size size) {
  if (color == ir_info::kNoColor) {
    common::fail("attempted to convert no color to operand");
  } else if (0 <= color && color <= 3) {
    return x86_64::Reg(size, color);
  } else if (4 <= color && color <= 13) {
    return x86_64::Reg(size, color + 2);
  } else {
    return x86_64::Mem::BasePointerDisp(size, int32_t(-8 * (color - 14)));
  }
}

ir_info::color_t OperandToColor(x86_64::RM operand) {
  if (operand.is_reg()) {
    int8_t reg = operand.reg().reg();
    if (0 <= reg && reg <= 3) {
      return ir_info::color_t(reg);
    } else if (6 <= reg && reg <= 15) {
      return ir_info::color_t(reg - 2);
    } else {
      common::fail("attempted to convert unexpected register to interference graph color");
    }
  } else if (operand.is_mem()) {
    int32_t disp = operand.mem().disp();
    return ir_info::color_t((disp / -8) + 14);
  } else {
    common::fail("attempted to convert unexpected x86_64 RM to interference graph color");
  }
}

namespace {

void AddPreferredColorsForFuncArgs(const ir::Func* func,
                                   ir_info::InterferenceGraphColors& preferred_colors) {
  for (size_t arg_index = 0; arg_index < func->args().size(); arg_index++) {
    ir::value_num_t arg_value = func->args().at(arg_index)->number();
    x86_64::RM arg_operand = OperandForArg(int(arg_index), x86_64::Size::k64);
    preferred_colors.SetColor(arg_value, OperandToColor(arg_operand));
  }
}

void AddPreferredColorsForFuncResults(const ir::ReturnInstr* return_instr,
                                      ir_info::InterferenceGraphColors& preferred_colors) {
  for (size_t result_index = 0; result_index < return_instr->args().size(); result_index++) {
    ir::value_num_t result_value;
    if (ir::Value* v = return_instr->args().at(result_index).get();
        v->kind() == ir::Value::Kind::kComputed) {
      result_value = static_cast<ir::Computed*>(v)->number();
    } else {
      continue;
    }
    x86_64::RM result_operand = OperandForResult(int(result_index), x86_64::Size::k64);
    preferred_colors.SetColor(result_value, OperandToColor(result_operand));
  }
}

const ir_info::InterferenceGraphColors AllocateRegistersInFunc(
    const ir::Func* func, const ir_info::InterferenceGraph& graph) {
  ir_info::InterferenceGraphColors preferred_colors;

  AddPreferredColorsForFuncArgs(func, preferred_colors);

  for (auto& block : func->blocks()) {
    ir::Instr* last_instr = block->instrs().back().get();
    if (last_instr->instr_kind() != ir::InstrKind::kReturn) {
      continue;
    }
    ir::ReturnInstr* return_instr = static_cast<ir::ReturnInstr*>(last_instr);

    AddPreferredColorsForFuncResults(return_instr, preferred_colors);
  }

  return ir_analyzers::ColorInterferenceGraph(graph, preferred_colors);
}

}  // namespace

std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraphColors> AllocateRegisters(
    const ir::Program* program,
    const std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraph>&
        interference_graphs) {
  std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraphColors>
      interference_graph_colors;
  interference_graph_colors.reserve(interference_graphs.size());
  for (auto& ir_func : program->funcs()) {
    interference_graph_colors.emplace(
        ir_func->number(),
        AllocateRegistersInFunc(ir_func.get(), interference_graphs.at(ir_func->number())));
  }
  return interference_graph_colors;
}

}  // namespace ir_to_x86_64_translator

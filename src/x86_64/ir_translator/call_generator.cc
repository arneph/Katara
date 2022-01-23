//
//  call_generator.cc
//  Katara
//
//  Created by Arne Philipeit on 1/23/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "call_generator.h"

#include "src/common/logging.h"
#include "src/x86_64/instrs/control_flow_instrs.h"
#include "src/x86_64/instrs/data_instrs.h"
#include "src/x86_64/ir_translator/mov_generator.h"
#include "src/x86_64/ir_translator/register_allocator.h"
#include "src/x86_64/ir_translator/size_translator.h"
#include "src/x86_64/ir_translator/value_translator.h"

namespace ir_to_x86_64_translator {
namespace {

std::vector<x86_64::Reg> GetCallerSavedRegisters(ir::Instr* instr, BlockContext& ctx) {
  std::vector<x86_64::Reg> caller_saved_registers;
  for (ir::value_num_t live_value : ctx.live_ranges().GetLiveSet(instr)) {
    ir_info::color_t color = ctx.func_ctx().interference_graph_colors().GetColor(live_value);
    x86_64::RM rm = ColorAndSizeToOperand(color, x86_64::k64);
    if (!rm.is_reg()) {
      continue;
    }
    x86_64::Reg reg = rm.reg();
    if (SavingBehaviourForReg(reg) != RegSavingBehaviour::kByCaller) {
      continue;
    }

    if (ctx.live_ranges().ValueDefinitionOf(live_value) == instr) {
      continue;
    }
    if (ctx.live_ranges().LastValueUseOf(live_value) == instr) {
      continue;
    }
    caller_saved_registers.push_back(reg);
  }
  std::sort(caller_saved_registers.begin(), caller_saved_registers.end(),
            [](x86_64::Reg reg_x, x86_64::Reg reg_y) { return reg_x.reg() <= reg_y.reg(); });
  return caller_saved_registers;
}

void GenerateCallerRegisterSaves(const std::vector<x86_64::Reg>& caller_saved_registers,
                                 BlockContext& ctx) {
  std::for_each(caller_saved_registers.begin(), caller_saved_registers.end(),
                [&](x86_64::Reg reg) { ctx.x86_64_block()->AddInstr<x86_64::Push>(reg); });
}

void GenerateCallerRegisterRestores(const std::vector<x86_64::Reg>& caller_saved_registers,
                                    BlockContext& ctx) {
  std::for_each(caller_saved_registers.rbegin(), caller_saved_registers.rend(),
                [&](x86_64::Reg reg) { ctx.x86_64_block()->AddInstr<x86_64::Pop>(reg); });
}

void GenerateArgMoves(ir::Instr* ir_instr, std::vector<ir::Value*> ir_args, BlockContext& ctx) {
  std::vector<MoveOperation> arg_moves;
  arg_moves.reserve(ir_args.size());
  for (std::size_t arg_index = 0; arg_index < ir_args.size(); arg_index++) {
    ir::Value* ir_arg_value = ir_args.at(arg_index);
    x86_64::Operand x86_64_arg_value =
        TranslateValue(ir_arg_value, IntNarrowing::kNone, ctx.func_ctx());
    x86_64::Size x86_64_arg_size = TranslateSizeOfType(ir_arg_value->type());
    x86_64::RM x86_64_arg_location = OperandForArg(int(arg_index), x86_64_arg_size);
    arg_moves.push_back(MoveOperation(x86_64_arg_location, x86_64_arg_value));
  }
  GenerateMovs(arg_moves, ir_instr, ctx);
}

void GenerateResultMoves(ir::Instr* ir_instr, std::vector<ir::Computed*> results,
                         BlockContext& ctx) {
  std::vector<MoveOperation> result_moves;
  result_moves.reserve(results.size());
  for (std::size_t result_index = 0; result_index < results.size(); result_index++) {
    ir::Computed* ir_result = results.at(result_index);
    x86_64::RM x86_64_result = TranslateComputed(ir_result, ctx.func_ctx());
    x86_64::Size x86_64_result_size = TranslateSizeOfType(ir_result->type());
    x86_64::RM x86_64_result_location = OperandForResult(int(result_index), x86_64_result_size);
    result_moves.push_back(MoveOperation(x86_64_result, x86_64_result_location));
  }
  GenerateMovs(result_moves, ir_instr, ctx);
}

void GenerateCallInstr(ir::Value* ir_called_func, BlockContext& ctx) {
  x86_64::Operand x86_64_called_func =
      TranslateValue(ir_called_func, IntNarrowing::k64To32BitIfPossible, ctx.func_ctx());
  if (x86_64_called_func.is_func_ref()) {
    ctx.x86_64_block()->AddInstr<x86_64::Call>(x86_64_called_func.func_ref());
  } else if (x86_64_called_func.is_rm()) {
    ctx.x86_64_block()->AddInstr<x86_64::Call>(x86_64_called_func.rm());
  } else {
    common::fail("unexpected func operand");
  }
}

}  // namespace

void GenerateCall(ir::Instr* ir_instr, ir::Value* ir_called_func,
                  std::vector<ir::Computed*> ir_results, std::vector<ir::Value*> ir_args,
                  BlockContext& ctx) {
  std::vector<x86_64::Reg> caller_saved_registers = GetCallerSavedRegisters(ir_instr, ctx);
  GenerateCallerRegisterSaves(caller_saved_registers, ctx);
  GenerateArgMoves(ir_instr, ir_args, ctx);
  GenerateCallInstr(ir_called_func, ctx);
  GenerateResultMoves(ir_instr, ir_results, ctx);
  GenerateCallerRegisterRestores(caller_saved_registers, ctx);
}

void GenerateCall(ir::Instr* ir_instr, x86_64::FuncRef x86_64_called_func,
                  std::vector<ir::Computed*> ir_results, std::vector<ir::Value*> ir_args,
                  BlockContext& ctx) {
  std::vector<x86_64::Reg> caller_saved_registers = GetCallerSavedRegisters(ir_instr, ctx);
  GenerateCallerRegisterSaves(caller_saved_registers, ctx);
  GenerateArgMoves(ir_instr, ir_args, ctx);
  ctx.x86_64_block()->AddInstr<x86_64::Call>(x86_64_called_func);
  GenerateResultMoves(ir_instr, ir_results, ctx);
  GenerateCallerRegisterRestores(caller_saved_registers, ctx);
}

}  // namespace ir_to_x86_64_translator

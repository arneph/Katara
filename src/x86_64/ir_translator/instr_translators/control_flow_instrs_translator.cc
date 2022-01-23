//
//  control_flow_instrs_translator.cc
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "control_flow_instrs_translator.h"

#include <cstdint>
#include <vector>

#include "src/common/logging.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/values.h"
#include "src/x86_64/instrs/arithmetic_logic_instrs.h"
#include "src/x86_64/instrs/control_flow_instrs.h"
#include "src/x86_64/instrs/data_instrs.h"
#include "src/x86_64/ir_translator/call_generator.h"
#include "src/x86_64/ir_translator/register_allocator.h"
#include "src/x86_64/ir_translator/size_translator.h"
#include "src/x86_64/ir_translator/value_translator.h"
#include "src/x86_64/ops.h"

namespace ir_to_x86_64_translator {

void TranslateJumpInstr(ir::JumpInstr* ir_jump_instr, BlockContext& ctx) {
  ir::block_num_t ir_destination = ir_jump_instr->destination();
  x86_64::BlockRef x86_64_destination = TranslateBlockValue(ir_destination, ctx.func_ctx());

  ctx.x86_64_block()->AddInstr<x86_64::Jmp>(x86_64_destination);
}

void TranslateJumpCondInstr(ir::JumpCondInstr* ir_jump_cond_instr, BlockContext& ctx) {
  auto ir_condition = ir_jump_cond_instr->condition().get();
  ir::block_num_t ir_destination_true = ir_jump_cond_instr->destination_true();
  ir::block_num_t ir_destination_false = ir_jump_cond_instr->destination_false();

  x86_64::BlockRef x86_64_destination_true =
      TranslateBlockValue(ir_destination_true, ctx.func_ctx());
  x86_64::BlockRef x86_64_destination_false =
      TranslateBlockValue(ir_destination_false, ctx.func_ctx());

  switch (ir_condition->kind()) {
    case ir::Value::Kind::kConstant: {
      auto ir_condition_constant = static_cast<ir::BoolConstant*>(ir_condition);
      if (ir_condition_constant->value()) {
        ctx.x86_64_block()->AddInstr<x86_64::Jmp>(x86_64_destination_true);
      } else {
        ctx.x86_64_block()->AddInstr<x86_64::Jmp>(x86_64_destination_false);
      }
      return;
    }
    case ir::Value::Kind::kComputed: {
      auto ir_condition_computed = static_cast<ir::Computed*>(ir_condition);
      x86_64::RM x86_64_condition = TranslateComputed(ir_condition_computed, ctx.func_ctx());

      // If x86_64_condition == 0x00, ZF gets set to 1.
      // If x86_64_condition != 0x00, ZF gets set to 0.
      // Therefore, counterintuitively x86_64_destination_true should be reached if ZF == 0.
      ctx.x86_64_block()->AddInstr<x86_64::Test>(x86_64_condition, x86_64::Imm(int8_t{-1}));
      ctx.x86_64_block()->AddInstr<x86_64::Jcc>(x86_64::InstrCond::kZero, x86_64_destination_true);
      ctx.x86_64_block()->AddInstr<x86_64::Jmp>(x86_64_destination_false);
      return;
    }
    case ir::Value::Kind::kInherited:
      common::fail("unexpected condition value kind");
  }
}

void TranslateCallInstr(ir::CallInstr* ir_call_instr, BlockContext& ctx) {
  std::vector<ir::Computed*> results;
  results.reserve(ir_call_instr->results().size());
  for (const auto& result : ir_call_instr->results()) {
    results.push_back(result.get());
  }
  std::vector<ir::Value*> args;
  args.reserve(ir_call_instr->args().size());
  for (const auto& arg : ir_call_instr->args()) {
    args.push_back(arg.get());
  }
  GenerateCall(ir_call_instr, ir_call_instr->func().get(), results, args, ctx);
}

void TranslateReturnInstr(ir::ReturnInstr* ir_return_instr, BlockContext& ctx) {
  struct ArgInfo {
    x86_64::Operand x86_64_arg_value;
    x86_64::RM x86_64_arg_location;
  };
  std::vector<ArgInfo> arg_infos;
  arg_infos.reserve(ir_return_instr->args().size());
  for (std::size_t arg_index = 0; arg_index < ir_return_instr->args().size(); arg_index++) {
    ir::Value* ir_arg_value = ir_return_instr->args().at(arg_index).get();
    x86_64::Operand x86_64_arg_value =
        TranslateValue(ir_arg_value, IntNarrowing::kNone, ctx.func_ctx());
    x86_64::Size x86_64_arg_size = TranslateSizeOfType(ir_arg_value->type());
    x86_64::RM x86_64_arg_location = OperandForResult(int(arg_index), x86_64_arg_size);
    arg_infos.push_back(ArgInfo{
        .x86_64_arg_value = x86_64_arg_value,
        .x86_64_arg_location = x86_64_arg_location,
    });
  }

  for (ArgInfo& arg_info : arg_infos) {
    if (arg_info.x86_64_arg_location != arg_info.x86_64_arg_value) {
      ctx.x86_64_block()->AddInstr<x86_64::Mov>(arg_info.x86_64_arg_location,
                                                arg_info.x86_64_arg_value);
    }
  }

  // TODO: improve
}

}  // namespace ir_to_x86_64_translator

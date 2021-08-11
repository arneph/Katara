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

      ctx.x86_64_block()->AddInstr<x86_64::Test>(x86_64_condition, x86_64::Imm(int8_t{-1}));
      ctx.x86_64_block()->AddInstr<x86_64::Jcc>(x86_64::InstrCond::kNoZero,
                                                x86_64_destination_true);
      ctx.x86_64_block()->AddInstr<x86_64::Jmp>(x86_64_destination_false);
      return;
    }
    case ir::Value::Kind::kInherited:
      common::fail("unexpected condition value kind");
  }
}

void TranslateCallInstr(ir::CallInstr* ir_call_instr, BlockContext& ctx) {
  struct ArgInfo {
    x86_64::Operand x86_64_arg_value;
    x86_64::RM x86_64_arg_location;
  };
  std::vector<ArgInfo> arg_infos;
  arg_infos.reserve(ir_call_instr->args().size());
  for (std::size_t arg_index = 0; arg_index < ir_call_instr->args().size(); arg_index++) {
    ir::Value* ir_arg_value = ir_call_instr->args().at(arg_index).get();
    x86_64::Operand x86_64_arg_value = TranslateValue(ir_arg_value, ctx.func_ctx());
    x86_64::Size x86_64_arg_size = TranslateSizeOfType(ir_arg_value->type());
    x86_64::RM x86_64_arg_location = OperandForArg(int(arg_index), x86_64_arg_size);
    arg_infos.push_back(ArgInfo{
        .x86_64_arg_value = x86_64_arg_value,
        .x86_64_arg_location = x86_64_arg_location,
    });
  }

  struct ResultInfo {
    x86_64::RM x86_64_result;
    x86_64::RM x86_64_result_location;
  };
  std::vector<ResultInfo> result_infos;
  result_infos.reserve(ir_call_instr->results().size());
  for (std::size_t result_index = 0; result_index < ir_call_instr->results().size();
       result_index++) {
    ir::Computed* ir_result = ir_call_instr->results().at(result_index).get();
    x86_64::RM x86_64_result = TranslateComputed(ir_result, ctx.func_ctx());
    x86_64::Size x86_64_result_size = TranslateSizeOfType(ir_result->type());
    x86_64::RM x86_64_result_location = OperandForResult(int(result_index), x86_64_result_size);
    result_infos.push_back(ResultInfo{
        .x86_64_result = x86_64_result,
        .x86_64_result_location = x86_64_result_location,
    });
  }

  for (ArgInfo& arg_info : arg_infos) {
    if (arg_info.x86_64_arg_location != arg_info.x86_64_arg_value) {
      ctx.x86_64_block()->AddInstr<x86_64::Mov>(arg_info.x86_64_arg_location,
                                                arg_info.x86_64_arg_value);
    }
  }

  ir::Value* ir_called_func = ir_call_instr->func().get();
  x86_64::Operand x86_64_called_func = TranslateValue(ir_called_func, ctx.func_ctx());
  if (x86_64_called_func.is_func_ref()) {
    ctx.x86_64_block()->AddInstr<x86_64::Call>(x86_64_called_func.func_ref());
  } else if (x86_64_called_func.is_rm()) {
    ctx.x86_64_block()->AddInstr<x86_64::Call>(x86_64_called_func.rm());
  } else {
    common::fail("unexpected func operand");
  }

  for (ResultInfo& result_info : result_infos) {
    if (result_info.x86_64_result_location != result_info.x86_64_result) {
      ctx.x86_64_block()->AddInstr<x86_64::Mov>(result_info.x86_64_result,
                                                result_info.x86_64_result_location);
    }
  }

  // TODO: improve
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
    x86_64::Operand x86_64_arg_value = TranslateValue(ir_arg_value, ctx.func_ctx());
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

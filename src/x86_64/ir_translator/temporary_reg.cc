//
//  temporary_reg.cc
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "temporary_reg.h"

#include <functional>
#include <unordered_set>

#include "src/common/logging.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/values.h"
#include "src/x86_64/instrs/data_instrs.h"
#include "src/x86_64/ir_translator/register_allocator.h"

namespace ir_to_x86_64_translator {

TemporaryReg TemporaryReg::ForOperand(x86_64::Operand operand, bool can_use_result_reg,
                                      const ir::Instr* instr, BlockContext& ctx) {
  TemporaryReg tmp = Prepare(operand.size(), can_use_result_reg, instr, ctx);
  ctx.x86_64_block()->AddInstr<x86_64::Mov>(tmp.reg(), operand);
  return tmp;
}

TemporaryReg TemporaryReg::Prepare(x86_64::Size x86_64_size, bool can_use_result_reg,
                                   const ir::Instr* instr, BlockContext& ctx) {
  if (can_use_result_reg) {
    if (std::optional<TemporaryReg> tmp = PrepareFromResultReg(x86_64_size, instr, ctx)) {
      return tmp.value();
    }
  }
  if (std::optional<TemporaryReg> tmp = PrepareFromUsedInFuncButNotLive(x86_64_size, instr, ctx)) {
    return tmp.value();
  }
  if (std::optional<TemporaryReg> tmp = PrepareFromUnusedInFunc(x86_64_size, ctx)) {
    return tmp.value();
  }
  if (std::optional<TemporaryReg> tmp =
          PrepareFromLiveButNotInvolvedInInstr(x86_64_size, instr, ctx)) {
    return tmp.value();
  }
  common::fail("temporary reg search exhausted all options");
}

std::optional<TemporaryReg> TemporaryReg::PrepareFromResultReg(x86_64::Size x86_64_size,
                                                               const ir::Instr* instr,
                                                               const BlockContext& ctx) {
  for (auto& result : instr->DefinedValues()) {
    ir::value_num_t result_num = result->number();
    ir_info::color_t result_color = ctx.func_ctx().interference_graph_colors().GetColor(result_num);
    x86_64::Operand tmp_op = ColorAndSizeToOperand(result_color, x86_64_size);
    if (!tmp_op.is_reg()) {
      continue;
    }
    bool result_color_is_also_arg_color = false;
    for (auto& arg : instr->UsedValues()) {
      if (arg->kind() != ir::Value::Kind::kComputed) {
        continue;
      }
      ir::value_num_t arg_num = static_cast<ir::Computed*>(arg.get())->number();
      ir_info::color_t arg_color = ctx.func_ctx().interference_graph_colors().GetColor(arg_num);
      if (result_color == arg_color) {
        result_color_is_also_arg_color = true;
        break;
      }
    }
    if (result_color_is_also_arg_color) {
      continue;
    }
    return TemporaryReg(tmp_op.reg(), RestorationState::kNotNeeded);
  }
  return std::nullopt;
}

std::optional<TemporaryReg> TemporaryReg::PrepareFromUsedInFuncButNotLive(x86_64::Size x86_64_size,
                                                                          const ir::Instr* instr,
                                                                          const BlockContext& ctx) {
  const std::unordered_set<ir::value_num_t>& value_live_set = ctx.live_ranges().GetLiveSet(instr);
  const std::unordered_set<ir_info::color_t> color_live_set =
      ctx.func_ctx().interference_graph_colors().GetColors(value_live_set);

  for (ir_info::color_t color : ctx.func_ctx().used_colors()) {
    x86_64::Operand tmp_op = ColorAndSizeToOperand(color, x86_64_size);
    if (!tmp_op.is_reg() || color_live_set.contains(color)) {
      continue;
    }
    return TemporaryReg(tmp_op.reg(), RestorationState::kNotNeeded);
  }
  return std::nullopt;
}

namespace {

std::optional<std::pair<x86_64::Reg, ir_info::color_t>> FindRegWithSizeAndColorIf(
    x86_64::Size x86_64_size, std::function<bool(ir_info::color_t)> f) {
  for (ir_info::color_t color = 0; true; color++) {
    x86_64::Operand tmp_op = ColorAndSizeToOperand(color, x86_64_size);
    if (!tmp_op.is_reg()) {
      break;
    }
    if (f(color)) {
      return std::pair<x86_64::Reg, ir_info::color_t>{tmp_op.reg(), color};
    }
  }
  return std::nullopt;
}

}  // namespace

std::optional<TemporaryReg> TemporaryReg::PrepareFromUnusedInFunc(x86_64::Size x86_64_size,
                                                                  BlockContext& ctx) {
  auto reg_and_color = FindRegWithSizeAndColorIf(x86_64_size, [ctx](ir_info::color_t color) {
    return !ctx.func_ctx().used_colors().contains(color);
  });
  if (reg_and_color) {
    auto [reg, color] = reg_and_color.value();
    ctx.func_ctx().AddUsedColor(color);
    return TemporaryReg(reg, RestorationState::kNotNeeded);
  }
  return std::nullopt;
}

namespace {

std::unordered_set<ir::value_num_t> ValuesInvolvedInInstr(const ir::Instr* instr) {
  std::unordered_set<ir::value_num_t> values;
  for (auto& defined : instr->DefinedValues()) {
    values.insert(defined->number());
  }
  for (auto& used : instr->DefinedValues()) {
    if (used->kind() != ir::Value::Kind::kComputed) {
      continue;
    }
    values.insert(static_cast<ir::Computed*>(used.get())->number());
  }
  return values;
}

}  // namespace

std::optional<TemporaryReg> TemporaryReg::PrepareFromLiveButNotInvolvedInInstr(
    x86_64::Size x86_64_size, const ir::Instr* instr, BlockContext& ctx) {
  const std::unordered_set<ir::value_num_t> involved_values_set = ValuesInvolvedInInstr(instr);
  const std::unordered_set<ir_info::color_t> involved_colors_set =
      ctx.func_ctx().interference_graph_colors().GetColors(involved_values_set);
  auto reg_and_color =
      FindRegWithSizeAndColorIf(x86_64_size, [&involved_colors_set](ir_info::color_t color) {
        return !involved_colors_set.contains(color);
      });
  if (reg_and_color) {
    auto [reg, color] = reg_and_color.value();
    ctx.func_ctx().AddUsedColor(color);
    ctx.x86_64_block()->AddInstr<x86_64::Push>(reg);
    return TemporaryReg(reg, RestorationState::kNeeded);
  }
  return std::nullopt;
}

TemporaryReg TemporaryReg::Prepare(x86_64::Reg tmp_reg, const ir::Instr* instr, BlockContext& ctx) {
  const ir_info::color_t tmp_color = OperandToColor(tmp_reg);

  if (!ctx.func_ctx().used_colors().contains(tmp_color)) {
    // Register was previously unused in the function.
    ctx.func_ctx().AddUsedColor(tmp_color);
    return TemporaryReg(tmp_reg, RestorationState::kNotNeeded);
  }

  const std::unordered_set<ir::value_num_t>& value_live_set = ctx.live_ranges().GetLiveSet(instr);
  const std::unordered_set<ir_info::color_t> color_live_set =
      ctx.func_ctx().interference_graph_colors().GetColors(value_live_set);

  if (!color_live_set.contains(tmp_color)) {
    // Register is not live during the instruction.
    return TemporaryReg(tmp_reg, RestorationState::kNotNeeded);
  } else {
    // Register is live during the instruction (and therefore needs preservation).
    ctx.x86_64_block()->AddInstr<x86_64::Push>(tmp_reg);
    return TemporaryReg(tmp_reg, RestorationState::kNeeded);
  }
}

void TemporaryReg::Restore(BlockContext& ctx) {
  switch (restoration_) {
    case RestorationState::kNeeded:
      ctx.x86_64_block()->AddInstr<x86_64::Pop>(reg_);
      return;
    case RestorationState::kNotNeeded:
      return;
  }
}

}  // namespace ir_to_x86_64_translator

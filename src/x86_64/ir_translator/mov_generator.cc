//
//  mov_generator.cc
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "mov_generator.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "src/common/logging/logging.h"
#include "src/x86_64/instrs/data_instrs.h"
#include "src/x86_64/instrs/instr.h"
#include "src/x86_64/ir_translator/size_translator.h"
#include "src/x86_64/ir_translator/temporary_reg.h"
#include "src/x86_64/ir_translator/value_translator.h"
#include "src/x86_64/ops.h"

namespace ir_to_x86_64_translator {

using ::common::logging::fail;

namespace {

bool IsNoOpMov(x86_64::RM x86_64_result, x86_64::Operand x86_64_origin) {
  return x86_64_result == x86_64_origin;
}

bool IsNoOpMov(MoveOperation& operation) {
  return IsNoOpMov(operation.result(), operation.origin());
}

bool MovNeedsTmpReg(x86_64::RM x86_64_result, x86_64::Operand x86_64_origin) {
  if (x86_64_result.is_reg()) {
    return false;
  }
  if (x86_64_origin.is_imm()) {
    return x86_64_origin.size() == x86_64::k64;
  } else if (x86_64_origin.is_mem()) {
    return true;
  }
  return false;
}

void GenerateMov(x86_64::RM x86_64_result, x86_64::Operand x86_64_origin, BlockContext& ctx,
                 std::function<x86_64::Reg(x86_64::Size)> temporary_reg_provider) {
  if (MovNeedsTmpReg(x86_64_result, x86_64_origin)) {
    x86_64::Reg tmp_reg = temporary_reg_provider(x86_64_origin.size());

    ctx.x86_64_block()->AddInstr<x86_64::Mov>(tmp_reg, x86_64_origin);
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(x86_64_result, tmp_reg);

  } else {
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(x86_64_result, x86_64_origin);
  }
}

void GenerateMov(MoveOperation& operation, BlockContext& ctx,
                 std::function<x86_64::Reg(x86_64::Size)> temporary_reg_provider) {
  GenerateMov(operation.result(), operation.origin(), ctx, temporary_reg_provider);
}

}  // namespace

void GenerateMov(x86_64::RM x86_64_result, x86_64::Operand x86_64_origin, ir::Instr* instr,
                 BlockContext& ctx) {
  if (IsNoOpMov(x86_64_result, x86_64_origin)) {
    return;
  }

  std::optional<TemporaryReg> tmp;
  auto temporary_reg_provider = [&tmp, &instr, &ctx](x86_64::Size size) {
    tmp = TemporaryReg::Prepare(size, /*can_be_result_reg=*/false, instr, ctx);
    return tmp->reg();
  };
  GenerateMov(x86_64_result, x86_64_origin, ctx, temporary_reg_provider);
  if (tmp.has_value()) {
    tmp->Restore(ctx);
  }
}

namespace {

void RemoveNoOps(std::vector<MoveOperation>& operations) {
  operations.erase(std::remove_if(operations.begin(), operations.end(),
                                  [](MoveOperation& operation) { return IsNoOpMov(operation); }),
                   operations.end());
}

struct ColorSets {
  std::unordered_set<ir_info::color_t> result_colors;
  std::unordered_set<ir_info::color_t> origin_colors;
};

ColorSets GetColorSets(const std::vector<MoveOperation>& operations) {
  ColorSets color_sets;
  for (const MoveOperation& operation : operations) {
    color_sets.result_colors.insert(operation.result_color());
    if (ir_info::color_t origin_color = operation.origin_color();
        origin_color != ir_info::kNoColor) {
      color_sets.origin_colors.insert(origin_color);
    }
  }
  return color_sets;
}

std::unordered_map<ir_info::color_t, int64_t> FindOriginColorUses(
    const std::vector<MoveOperation>& operations) {
  std::unordered_map<ir_info::color_t, int64_t> origin_color_uses;
  for (const MoveOperation& operation : operations) {
    ir_info::color_t origin_color = operation.origin_color();
    if (origin_color == ir_info::kNoColor) {
      continue;
    }
    origin_color_uses.insert({origin_color, 0});  // Note: only inserts if not present.
    origin_color_uses[origin_color]++;
  }
  return origin_color_uses;
}

std::unordered_set<ir_info::color_t> FindInitialFreeInvolvedRegColors(const ColorSets& color_sets) {
  std::unordered_set<ir_info::color_t> free_involved_reg_colors;
  for (ir_info::color_t color : color_sets.result_colors) {
    if (!color_sets.origin_colors.contains(color) &&
        ColorAndSizeToOperand(color, x86_64::k64).is_reg()) {
      free_involved_reg_colors.insert(color);
    }
  }
  return free_involved_reg_colors;
}

x86_64::Reg FindUninvolvedReg(const ColorSets& color_sets) {
  for (ir_info::color_t color = 0; true; color++) {
    if (color_sets.result_colors.contains(color) || color_sets.origin_colors.contains(color)) {
      continue;
    }
    x86_64::Operand operand = ColorAndSizeToOperand(color, x86_64::k64);
    if (operand.is_reg()) {
      return operand.reg();
    }
    fail("failed to find uninvolved reg");
  }
}

bool IsReady(MoveOperation& operation,
             const std::unordered_map<ir_info::color_t, int64_t>& remaining_origin_color_uses) {
  auto it = remaining_origin_color_uses.find(operation.result_color());
  if (it != remaining_origin_color_uses.end()) {
    return it->second == 0;
  } else {
    return true;
  }
}

bool ReducesFreeInvolvedRegs(MoveOperation& move_operation,
                             const std::unordered_set<ir_info::color_t>& free_involved_reg_colors) {
  return free_involved_reg_colors.contains(move_operation.result_color());
}

std::optional<MoveOperation> NextMoveOperation(
    std::vector<MoveOperation>& move_operations,
    const std::unordered_map<ir_info::color_t, int64_t>& remaining_origin_color_uses,
    const std::unordered_set<ir_info::color_t>& free_involved_reg_colors) {
  if (move_operations.empty()) {
    return std::nullopt;
  }
  auto move_operation_it =
      std::min_element(move_operations.begin(), move_operations.end(),
                       [&remaining_origin_color_uses, &free_involved_reg_colors](
                           MoveOperation& op_a, MoveOperation& op_b) {
                         // Returns true if op_a should be handled before op_b.
                         if (!IsReady(op_a, remaining_origin_color_uses)) {
                           return false;
                         } else if (!IsReady(op_b, remaining_origin_color_uses)) {
                           return true;
                         }
                         bool op_a_reduces_free_involved_regs =
                             ReducesFreeInvolvedRegs(op_a, free_involved_reg_colors);
                         bool op_b_reduces_free_involved_regs =
                             ReducesFreeInvolvedRegs(op_b, free_involved_reg_colors);
                         if (op_a_reduces_free_involved_regs != op_b_reduces_free_involved_regs) {
                           return !op_a_reduces_free_involved_regs;
                         }
                         return true;
                       });
  MoveOperation move_operation = *move_operation_it;
  if (IsReady(move_operation, remaining_origin_color_uses)) {
    move_operations.erase(move_operation_it);
    return move_operation;
  }
  return std::nullopt;
}

class MoveCycle {
 public:
  explicit MoveCycle(std::vector<x86_64::RM> operands) : operands_(operands) {}

  const std::vector<x86_64::RM>& operands() const { return operands_; }

  void RemoveOperand(x86_64::RM removed_operand) {
    operands_.erase(std::remove_if(operands_.begin(), operands_.end(),
                                   [&removed_operand](x86_64::RM& operand) {
                                     return removed_operand == operand;
                                   }),
                    operands_.end());
  }
  void RemoveAllOperands() { operands_.clear(); }

 private:
  std::vector<x86_64::RM> operands_;
};

// Moves all MoveOperations that do not participate in cycles to the beginning of operations and
// MoveOperations that do pariticpate towards the end. Returns an iterator pointing to the first
// MoveOperation that participates in a cycle (or the end of operations).
std::vector<MoveOperation>::iterator PartitionMoveOperationsByCycleParticipation(
    std::vector<MoveOperation>& operations,
    std::unordered_map<ir_info::color_t, int64_t> origin_color_uses) {
  bool reduced_cycle_participants = true;
  auto cycle_participants_start = operations.begin();
  while (reduced_cycle_participants) {
    auto new_cycle_participants_start = std::partition(
        cycle_participants_start, operations.end(), [&origin_color_uses](MoveOperation& operation) {
          auto it = origin_color_uses.find(operation.result_color());
          if (it != origin_color_uses.end()) {
            return it->second == 0;
          } else {
            return true;
          }
        });
    reduced_cycle_participants = (new_cycle_participants_start != cycle_participants_start);
    std::for_each(cycle_participants_start, new_cycle_participants_start,
                  [&origin_color_uses](MoveOperation& operation) {
                    auto it = origin_color_uses.find(operation.origin_color());
                    if (it != origin_color_uses.end()) {
                      it->second--;
                    }
                  });
    cycle_participants_start = new_cycle_participants_start;
  }
  return cycle_participants_start;
}

// Removes all MoveOperations that participate in the same cycle as the MoveOperation pointed to by
// cycle_start from operations and returns a MoveCycle containing the operands of all removed
// MoveOperations. Because operations gets shrunk, cycle_start and all other iterators get
// invalidated.
MoveCycle RemoveMoveOperationsInMoveCycle(
    std::vector<MoveOperation>& operations,
    std::vector<MoveOperation>::iterator cycle_participants_start,
    std::vector<MoveOperation>::iterator cycle_start) {
  ir_info::color_t cycle_start_color = cycle_start->origin_color();
  ir_info::color_t current_color = cycle_start->result_color();
  std::vector<x86_64::RM> cycle_operands{cycle_start->result()};
  operations.erase(cycle_start);
  while (cycle_start_color != current_color) {
    auto next_it = std::find_if(cycle_participants_start, operations.end(),
                                [current_color](MoveOperation& operation) {
                                  return current_color == operation.origin_color();
                                });
    if (next_it == operations.end()) {
      fail("move cycle is incomplete");
    }
    cycle_operands.push_back(next_it->result());
    current_color = next_it->result_color();
    operations.erase(next_it);
  }
  return MoveCycle(cycle_operands);
}

// Removes all MoveOperations that participate in cycles and returns MoveCycles containing the
// operands of all removed MoveOperations.
std::vector<MoveCycle> RemoveMoveOperationsInMoveCycles(
    std::vector<MoveOperation>& operations,
    std::unordered_map<ir_info::color_t, int64_t> origin_color_uses) {
  auto cycle_participants_start =
      PartitionMoveOperationsByCycleParticipation(operations, origin_color_uses);
  std::size_t non_cycle_participants = std::distance(operations.begin(), cycle_participants_start);
  std::vector<MoveCycle> move_cycles;
  while (non_cycle_participants < operations.size()) {
    auto cycle_start = operations.begin() + non_cycle_participants;
    move_cycles.push_back(
        RemoveMoveOperationsInMoveCycle(operations, cycle_participants_start, cycle_start));
  }
  return move_cycles;
}

class SwapOperation {
 public:
  SwapOperation(x86_64::RM operand_a, x86_64::RM operand_b)
      : operand_a_(operand_a), operand_b_(operand_b) {}

  x86_64::RM operand_a() const { return operand_a_; }
  x86_64::RM operand_b() const { return operand_b_; }

 private:
  x86_64::RM operand_a_;
  x86_64::RM operand_b_;
};

void GenerateXchg(SwapOperation& operation, BlockContext& ctx,
                  std::function<x86_64::Reg(x86_64::Size)> temporary_reg_provider) {
  x86_64::Size size = x86_64::Max(operation.operand_a().size(), operation.operand_b().size());
  x86_64::RM operand_a = x86_64::Resize(operation.operand_a(), size);
  x86_64::RM operand_b = x86_64::Resize(operation.operand_b(), size);
  if (operand_a.is_mem() && operand_b.is_mem()) {
    x86_64::Reg tmp_reg = temporary_reg_provider(size);

    ctx.x86_64_block()->AddInstr<x86_64::Mov>(tmp_reg, operand_a);
    ctx.x86_64_block()->AddInstr<x86_64::Xchg>(operand_b, tmp_reg);
    ctx.x86_64_block()->AddInstr<x86_64::Mov>(operand_a, tmp_reg);

  } else {
    if (operand_b.is_mem()) {
      std::swap(operand_a, operand_b);
    }
    ctx.x86_64_block()->AddInstr<x86_64::Xchg>(operand_a, operand_b.reg());
  }
}

std::optional<SwapOperation> NextSwapOperationForMoveCycle(
    MoveCycle& move_cycle,
    const std::unordered_map<ir_info::color_t, int64_t>& remaining_origin_color_uses) {
  for (std::size_t i = 0; i < move_cycle.operands().size(); i++) {
    std::size_t j = (i + 1) % move_cycle.operands().size();
    x86_64::RM operand_a = move_cycle.operands().at(i);
    x86_64::RM operand_b = move_cycle.operands().at(j);
    if (remaining_origin_color_uses.at(OperandToColor(operand_a)) > 1 ||
        remaining_origin_color_uses.at(OperandToColor(operand_b)) > 1) {
      continue;
    }
    if (move_cycle.operands().size() == 2) {
      move_cycle.RemoveAllOperands();
    } else {
      move_cycle.RemoveOperand(operand_b);
    }
    return SwapOperation(operand_a, operand_b);
  }
  return std::nullopt;
}

std::optional<SwapOperation> NextSwapOperation(
    std::vector<MoveCycle>& move_cycles,
    const std::unordered_map<ir_info::color_t, int64_t>& remaining_origin_color_uses) {
  for (auto it = move_cycles.begin(); it != move_cycles.end(); ++it) {
    MoveCycle& move_cycle = *it;
    std::optional<SwapOperation> swap_operation =
        NextSwapOperationForMoveCycle(move_cycle, remaining_origin_color_uses);
    if (swap_operation.has_value()) {
      if (move_cycle.operands().empty()) {
        it = move_cycles.erase(it);
      }
      return swap_operation;
    }
  }
  return std::nullopt;
}

}  // namespace

void GenerateMovs(std::vector<MoveOperation> move_operations, ir::Instr* instr, BlockContext& ctx) {
  RemoveNoOps(move_operations);
  if (move_operations.empty()) {
    return;
  }

  const ColorSets color_sets = GetColorSets(move_operations);
  std::unordered_map<ir_info::color_t, int64_t> remaining_origin_color_uses =
      FindOriginColorUses(move_operations);
  std::unordered_set<ir_info::color_t> free_involved_reg_colors =
      FindInitialFreeInvolvedRegColors(color_sets);

  std::vector<MoveCycle> move_cycles =
      RemoveMoveOperationsInMoveCycles(move_operations, remaining_origin_color_uses);

  std::optional<TemporaryReg> tmp;
  auto temporary_reg_provider = [&free_involved_reg_colors, &tmp, &color_sets, instr,
                                 &ctx](x86_64::Size size) {
    if (!free_involved_reg_colors.empty()) {
      return ColorAndSizeToOperand(*free_involved_reg_colors.begin(), size).reg();
    } else {
      if (!tmp.has_value()) {
        tmp = TemporaryReg::Prepare(FindUninvolvedReg(color_sets), instr, ctx);
      }
      return ColorAndSizeToOperand(tmp->reg().reg(), size).reg();
    }
  };

  while (!move_operations.empty() || !move_cycles.empty()) {
    std::optional<MoveOperation> move_operation =
        NextMoveOperation(move_operations, remaining_origin_color_uses, free_involved_reg_colors);
    if (move_operation.has_value()) {
      GenerateMov(move_operation.value(), ctx, temporary_reg_provider);
      if (move_operation->origin_color() != ir_info::kNoColor) {
        remaining_origin_color_uses[move_operation->origin_color()]--;
      }
      free_involved_reg_colors.erase(move_operation->result_color());
      continue;
    }
    std::optional<SwapOperation> swap_operation =
        NextSwapOperation(move_cycles, remaining_origin_color_uses);
    if (swap_operation.has_value()) {
      GenerateXchg(swap_operation.value(), ctx, temporary_reg_provider);
      continue;
    }
    fail("could not find any ready move or swap operation");
  }

  if (tmp.has_value()) {
    tmp->Restore(ctx);
  }
}

}  // namespace ir_to_x86_64_translator

//
//  live_range_analyzer.cc
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "live_range_analyzer.h"

#include "src/ir/representation/instrs.h"
#include "src/ir/representation/values.h"

namespace ir_proc {
namespace {

void BacktraceBlock(const ir::Func* func, const ir::Block* block,
                    ir_info::BlockLiveRanges& live_ranges) {
  const size_t n = block->instrs().size();

  // Backtrace through instructions in block
  // Add value defintions and uses (outside phi instructions)
  for (int64_t index = n - 1; index >= 0; index--) {
    ir::Instr* instr = block->instrs().at(index).get();

    for (auto& defined_value : instr->DefinedValues()) {
      live_ranges.AddValueDefinition(defined_value->number(), index);
    }

    if (instr->instr_kind() == ir::InstrKind::kPhi) {
      continue;
    }
    for (auto& used_value : instr->UsedValues()) {
      if (used_value->kind() != ir::Value::Kind::kComputed) {
        continue;
      }
      auto used_computed = static_cast<ir::Computed*>(used_value.get());
      live_ranges.AddValueUse(used_computed->number(), index);
    }
  }

  // Include values used in phi instructions of child
  for (ir::block_num_t child_num : block->children()) {
    func->GetBlock(child_num)->for_each_phi_instr([&](ir::PhiInstr* instr) {
      ir::Value* value = instr->ValueInheritedFromBlock(block->number()).get();
      if (value->kind() != ir::Value::Kind::kComputed) {
        return;
      }
      auto computed = static_cast<ir::Computed*>(value);
      live_ranges.PropagateBackwardsFromExitSet(computed->number());
    });
  }
}

}  // namespace

const ir_info::FuncLiveRanges FindLiveRangesForFunc(const ir::Func* func) {
  ir_info::FuncLiveRanges func_live_ranges(func);
  std::unordered_set<ir::block_num_t> queue;

  for (auto& block : func->blocks()) {
    ir_info::BlockLiveRanges& block_live_ranges =
        func_live_ranges.GetBlockLiveRanges(block->number());

    BacktraceBlock(func, block.get(), block_live_ranges);

    if (!block_live_ranges.GetEntrySet().empty()) {
      queue.insert(block->number());
    }
  }

  while (!queue.empty()) {
    auto it = queue.begin();
    ir::block_num_t bnum = *it;
    queue.erase(it);

    ir_info::BlockLiveRanges& block_live_ranges = func_live_ranges.GetBlockLiveRanges(bnum);

    for (ir::block_num_t parent_num : func->GetBlock(bnum)->parents()) {
      ir_info::BlockLiveRanges& parent_live_ranges =
          func_live_ranges.GetBlockLiveRanges(parent_num);

      size_t old_entry_set_size = parent_live_ranges.GetEntrySet().size();

      for (ir::value_num_t value : block_live_ranges.GetEntrySet()) {
        parent_live_ranges.PropagateBackwardsFromExitSet(value);
      }

      size_t new_entry_set_size = parent_live_ranges.GetEntrySet().size();

      if (old_entry_set_size < new_entry_set_size) {
        queue.insert(parent_num);
      }
    }
  }

  return func_live_ranges;
}

}  // namespace ir_proc

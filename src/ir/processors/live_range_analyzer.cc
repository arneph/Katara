//
//  live_range_analyzer.cc
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "live_range_analyzer.h"

namespace ir_proc {

ir_info::FuncLiveRanges& LiveRangeAnalyzer::func_info() {
  FindLiveRanges();

  return func_info_;
}

ir_info::InterferenceGraph& LiveRangeAnalyzer::interference_graph() {
  FindLiveRanges();
  BuildInterferenceGraph();

  return interference_graph_;
}

void LiveRangeAnalyzer::FindLiveRanges() {
  if (func_info_ok_) return;
  func_info_ok_ = true;

  std::unordered_set<ir::block_num_t> queue;

  for (auto& block : func_->blocks()) {
    ir_info::BlockLiveRanges& block_info = func_info_.GetBlockLiveRanges(block->number());

    BacktraceBlock(block.get(), block_info);

    if (!block_info.GetEntrySet().empty()) {
      queue.insert(block->number());
    }
  }

  while (!queue.empty()) {
    auto it = queue.begin();
    ir::block_num_t bnum = *it;
    queue.erase(it);

    ir_info::BlockLiveRanges& block_info = func_info_.GetBlockLiveRanges(bnum);

    for (ir::block_num_t parent_num : func_->GetBlock(bnum)->parents()) {
      ir_info::BlockLiveRanges& parent_info = func_info_.GetBlockLiveRanges(parent_num);

      size_t old_entry_set_size = parent_info.GetEntrySet().size();

      for (ir::value_num_t value : block_info.GetEntrySet()) {
        parent_info.PropagateBackwardsFromExitSet(value);
      }

      size_t new_entry_set_size = parent_info.GetEntrySet().size();

      if (old_entry_set_size < new_entry_set_size) {
        queue.insert(parent_num);
      }
    }
  }
}

void LiveRangeAnalyzer::BacktraceBlock(ir::Block* block, ir_info::BlockLiveRanges& info) {
  const size_t n = block->instrs().size();

  // Backtrace through instructions in block
  // Add value defintions and uses (outside phi instructions)
  for (int64_t index = n - 1; index >= 0; index--) {
    ir::Instr* instr = block->instrs().at(index).get();

    for (auto& defined_value : instr->DefinedValues()) {
      info.AddValueDefinition(defined_value->number(), index);
    }

    if (dynamic_cast<ir::PhiInstr*>(instr) != nullptr) {
      continue;
    }
    for (auto& used_value : instr->UsedValues()) {
      auto used_computed_value = std::dynamic_pointer_cast<ir::Computed>(used_value);
      if (used_computed_value == nullptr) {
        continue;
      }
      info.AddValueUse(used_computed_value->number(), index);
    }
  }

  // Include values used in phi instructions of child
  for (ir::block_num_t child_num : block->children()) {
    func_->GetBlock(child_num)->for_each_phi_instr([&](ir::PhiInstr* instr) {
      std::shared_ptr<ir::Value> value = instr->ValueInheritedFromBlock(block->number());
      auto computed = std::dynamic_pointer_cast<ir::Computed>(value);
      if (computed == nullptr) return;

      info.PropagateBackwardsFromExitSet(computed->number());
    });
  }
}

void LiveRangeAnalyzer::BuildInterferenceGraph() {
  if (interference_graph_ok_) return;
  interference_graph_ok_ = true;

  FindLiveRanges();

  for (auto& block : func_->blocks()) {
    ir_info::BlockLiveRanges& block_info = func_info_.GetBlockLiveRanges(block->number());

    BuildInterferenceGraph(block.get(), block_info);
  }
}

void LiveRangeAnalyzer::BuildInterferenceGraph(ir::Block* block, ir_info::BlockLiveRanges& info) {
  const size_t n = block->instrs().size();
  std::unordered_set<ir::value_num_t> live_set = info.GetExitSet();

  interference_graph_.AddEdgesIn(live_set);

  for (int64_t i = n - 1; i >= 0; i--) {
    ir::Instr* instr = block->instrs().at(i).get();
    bool is_phi = (dynamic_cast<ir::PhiInstr*>(instr) != nullptr);

    for (auto& defined_value : instr->DefinedValues()) {
      auto it = live_set.find(defined_value->number());
      if (it == live_set.end()) {
        interference_graph_.AddEdgesBetween(live_set, defined_value->number());

      } else {
        live_set.erase(it);
      }
    }

    for (auto& used_value : instr->UsedValues()) {
      auto used_computed_value = std::dynamic_pointer_cast<ir::Computed>(used_value);
      if (used_computed_value == nullptr) {
        continue;
      }
      auto it = live_set.find(used_computed_value->number());
      if (it == live_set.end()) {
        interference_graph_.AddEdgesBetween(live_set, used_computed_value->number());

        if (!is_phi) {
          live_set.insert(used_computed_value->number());
        }
      }
    }
  }
}

}  // namespace ir_proc

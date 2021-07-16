//
//  interference_graph_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 7/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "interference_graph_builder.h"

namespace ir_proc {
namespace {

void PopulateInterferenceGraphForBlock(const ir::Block* block, const ir_info::BlockLiveRanges& info,
                                       ir_info::InterferenceGraph& graph) {
  const size_t n = block->instrs().size();
  std::unordered_set<ir::value_num_t> live_set = info.GetExitSet();

  graph.AddEdgesIn(live_set);

  for (int64_t i = n - 1; i >= 0; i--) {
    ir::Instr* instr = block->instrs().at(i).get();
    bool is_phi = (instr->instr_kind() == ir::InstrKind::kPhi);

    for (auto& defined_value : instr->DefinedValues()) {
      auto it = live_set.find(defined_value->number());
      if (it == live_set.end()) {
        graph.AddEdgesBetween(live_set, defined_value->number());
      } else {
        live_set.erase(it);
      }
    }

    for (auto& used_value : instr->UsedValues()) {
      if (used_value->kind() != ir::Value::Kind::kComputed) {
        continue;
      }
      auto used_computed = static_cast<ir::Computed*>(used_value.get());
      auto it = live_set.find(used_computed->number());
      if (it == live_set.end()) {
        graph.AddEdgesBetween(live_set, used_computed->number());

        if (!is_phi) {
          live_set.insert(used_computed->number());
        }
      }
    }
  }
}

}  // namespace

const ir_info::InterferenceGraph BuildInterferenceGraphForFunc(
    const ir::Func* func, const ir_info::FuncLiveRanges func_live_ranges) {
  ir_info::InterferenceGraph graph;

  for (auto& block : func->blocks()) {
    PopulateInterferenceGraphForBlock(block.get(),
                                      func_live_ranges.GetBlockLiveRanges(block->number()), graph);
  }

  return graph;
}

}  // namespace ir_proc

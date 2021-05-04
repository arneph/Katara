//
//  register_allocator.cc
//  Katara
//
//  Created by Arne Philipeit on 1/23/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "register_allocator.h"

#include <queue>

namespace ir_proc {

RegisterAllocator::RegisterAllocator(ir::Func* /*func*/, ir_info::InterferenceGraph& graph)
    :  // func_(func),
      graph_(graph) {}

RegisterAllocator::~RegisterAllocator() {}

void RegisterAllocator::AllocateRegisters() {
  /*
  std::unordered_map<ir::Computed, size_t> neighbor_count;
  std::unordered_map<ir::Computed, size_t> constraints_count;

  neighbor_count.reserve(graph_.values().size());
  constraints_count.reserve(graph_.values().size());

  for (ir::Computed value : graph_.values()) {
      size_t c = graph_.GetNeighbors(value).size();

      neighbor_count.insert({value, c});
      constraints_count.insert({value, c});
  }

  auto queue_compare = [] (ir::Computed a, ir::Computed b) {

  };

  std::queue<ir::Computed,
             std::vector<ir::Computed>,
             queue_compare> queue;*/

  for (ir::value_num_t value : graph_.values()) {
    if (graph_.GetRegister(value) != -1) {
      continue;
    }

    std::unordered_set<int64_t> neighbor_colors;

    for (ir::value_num_t neighbor : graph_.GetNeighbors(value)) {
      int64_t neighbor_color = graph_.GetRegister(neighbor);

      neighbor_colors.insert(neighbor_color);
    }

    for (int64_t c = 0; c < int64_t(neighbor_colors.size() + 1); c++) {
      if (neighbor_colors.count(c) == 0) {
        graph_.SetRegister(value, c);
        break;
      }
    }
  }
}

}  // namespace ir_proc

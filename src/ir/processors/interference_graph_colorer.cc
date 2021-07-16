//
//  interference_graph_colorer.cc
//  Katara
//
//  Created by Arne Philipeit on 1/23/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "interference_graph_colorer.h"

namespace ir_proc {

const ir_info::InterferenceGraphColors ColorInterferenceGraph(
    const ir_info::InterferenceGraph& graph,
    const ir_info::InterferenceGraphColors& preferred_colors) {
  // Idea: optimize to make as many movs no ops as possible
  // Idea: optimize to use least number of colors
  // Idea: optimize to satisfy as many preferred colors as possible
  ir_info::InterferenceGraphColors result_colors;

  for (ir::value_num_t value : graph.values()) {
    ir_info::color_t preferred_color = preferred_colors.GetColor(value);
    std::unordered_set<ir_info::color_t> neighbor_colors;
    for (ir::value_num_t neighbor : graph.GetNeighbors(value)) {
      neighbor_colors.insert(result_colors.GetColor(neighbor));
    }

    if (preferred_color != ir_info::kNoColor && !neighbor_colors.contains(preferred_color)) {
      result_colors.SetColor(value, preferred_color);
      continue;
    }

    for (ir_info::color_t color = 0; color < ir_info::color_t(neighbor_colors.size() + 1);
         color++) {
      if (neighbor_colors.count(color) == 0) {
        result_colors.SetColor(value, color);
        break;
      }
    }
  }

  return result_colors;
}

}  // namespace ir_proc

//
//  interference_graph.h
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_info_interference_graph_h
#define ir_info_interference_graph_h

#include <unordered_map>
#include <unordered_set>

#include "src/common/graph.h"
#include "src/ir/representation/num_types.h"

namespace ir_info {

class InterferenceGraph {
 public:
  const std::unordered_set<ir::value_num_t>& values() const { return values_; }
  const std::unordered_set<ir::value_num_t>& GetNeighbors(ir::value_num_t value) const {
    return graph_.at(value);
  }

  void AddValue(ir::value_num_t value);
  void AddEdge(ir::value_num_t value_a, ir::value_num_t value_b);
  void AddEdgesIn(std::unordered_set<ir::value_num_t> group);
  void AddEdgesBetween(std::unordered_set<ir::value_num_t> group, ir::value_num_t individual);

  std::string ToString() const;
  common::Graph ToGraph(const class InterferenceGraphColors* colors = nullptr) const;

 private:
  std::unordered_set<ir::value_num_t> values_;
  std::unordered_map<ir::value_num_t, std::unordered_set<ir::value_num_t>> graph_;
};

typedef int64_t color_t;
constexpr color_t kNoColor = -1;

class InterferenceGraphColors {
 public:
  color_t GetColor(ir::value_num_t value) const;
  std::unordered_set<ir_info::color_t> GetColors(
      const std::unordered_set<ir::value_num_t>& values) const;

  void SetColor(ir::value_num_t value, color_t color) { colors_.insert({value, color}); }

  std::string ToString() const;

 private:
  std::unordered_map<ir::value_num_t, color_t> colors_;
};

}  // namespace ir_info

#endif /* ir_info_interference_graph_h */

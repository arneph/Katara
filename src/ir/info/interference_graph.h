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

#include "common/graph.h"
#include "ir/representation/num_types.h"

namespace ir_info {

class InterferenceGraph {
 public:
  InterferenceGraph() {}

  const std::unordered_set<ir::value_num_t>& values() const { return values_; }
  const std::unordered_set<ir::value_num_t>& GetNeighbors(ir::value_num_t value) const {
    return graph_.at(value);
  }

  void AddValue(ir::value_num_t value);
  void AddEdge(ir::value_num_t value_a, ir::value_num_t value_b);
  void AddEdgesIn(std::unordered_set<ir::value_num_t> group);
  void AddEdgesBetween(std::unordered_set<ir::value_num_t> group, ir::value_num_t individual);

  int64_t GetRegister(ir::value_num_t value) const;
  void SetRegister(ir::value_num_t value, int64_t reg);
  void ResetRegisters();

  std::string ToString() const;
  common::Graph ToVCGGraph() const;

 private:
  std::unordered_set<ir::value_num_t> values_;
  std::unordered_map<ir::value_num_t, std::unordered_set<ir::value_num_t>> graph_;
  std::unordered_map<ir::value_num_t, int64_t> regs_;
};

}  // namespace ir_info

#endif /* ir_info_interference_graph_h */

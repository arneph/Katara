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

#include "ir/representation/value.h"
#include "vcg/graph.h"

namespace ir_info {

class InterferenceGraph {
 public:
  InterferenceGraph();
  ~InterferenceGraph();

  const std::unordered_set<ir::Computed>& values() const;
  const std::unordered_set<ir::Computed>& GetNeighbors(ir::Computed value) const;

  void AddValue(ir::Computed value);
  void AddEdge(ir::Computed value_a, ir::Computed value_b);
  void AddEdgesIn(std::unordered_set<ir::Computed> group);
  void AddEdgesBetween(std::unordered_set<ir::Computed> group, ir::Computed individual);

  int64_t GetRegister(ir::Computed value) const;
  void SetRegister(ir::Computed value, int64_t reg);
  void ResetRegisters();

  std::string ToString() const;
  vcg::Graph ToVCGGraph() const;

 private:
  std::unordered_set<ir::Computed> values_;
  std::unordered_map<ir::Computed, std::unordered_set<ir::Computed>> graph_;
  std::unordered_map<ir::Computed, int64_t> regs_;
};

}  // namespace ir_info

#endif /* ir_info_interference_graph_h */

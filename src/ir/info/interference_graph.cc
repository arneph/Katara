//
//  interference_graph.cc
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "interference_graph.h"

#include <iomanip>
#include <sstream>
#include <vector>

namespace ir_info {

void InterferenceGraph::AddValue(ir::value_num_t value) {
  auto it = values_.find(value);
  if (it != values_.end()) {
    return;
  }

  values_.insert(value);
  graph_.insert({value, std::unordered_set<ir::value_num_t>()});
  regs_.insert({value, -1});
}

void InterferenceGraph::AddEdge(ir::value_num_t value_a, ir::value_num_t value_b) {
  auto it_a = graph_.find(value_a);
  if (it_a != graph_.end()) {
    it_a->second.insert(value_b);
  } else {
    values_.insert(value_a);
    graph_.insert({value_a, std::unordered_set<ir::value_num_t>{value_b}});
    regs_.insert({value_a, -1});
  }

  auto it_b = graph_.find(value_b);
  if (it_b != graph_.end()) {
    it_b->second.insert(value_a);
  } else {
    values_.insert(value_b);
    graph_.insert({value_b, std::unordered_set<ir::value_num_t>{value_a}});
    regs_.insert({value_b, -1});
  }
}

void InterferenceGraph::AddEdgesIn(std::unordered_set<ir::value_num_t> group) {
  if (group.size() == 0) return;
  if (group.size() == 1) {
    AddValue(*group.begin());
    return;
  }

  for (ir::value_num_t group_member : group) {
    auto it = graph_.find(group_member);
    if (it == graph_.end()) {
      values_.insert(group_member);
      auto result = graph_.insert({group_member, std::unordered_set<ir::value_num_t>()});
      regs_.insert({group_member, -1});

      it = result.first;
    }

    std::unordered_set<ir::value_num_t>& neighbors = it->second;

    for (ir::value_num_t other : group) {
      if (group_member == other) continue;

      neighbors.insert(other);
    }
  }
}

void InterferenceGraph::AddEdgesBetween(std::unordered_set<ir::value_num_t> group,
                                        ir::value_num_t individual) {
  for (ir::value_num_t group_member : group) {
    auto it = graph_.find(group_member);
    if (it != graph_.end()) {
      it->second.insert(individual);
    } else {
      values_.insert(group_member);
      graph_.insert({group_member, std::unordered_set<ir::value_num_t>{individual}});
      regs_.insert({group_member, -1});
    }
  }

  auto it = graph_.find(individual);
  if (it != graph_.end()) {
    it->second.insert(group.begin(), group.end());
  } else {
    values_.insert(individual);
    graph_.insert({individual, group});
    regs_.insert({individual, -1});
  }
}

int64_t InterferenceGraph::GetRegister(ir::value_num_t value) const { return regs_.at(value); }

void InterferenceGraph::SetRegister(ir::value_num_t value, int64_t reg) { regs_[value] = reg; }

void InterferenceGraph::ResetRegisters() {
  for (auto& [value, reg] : regs_) {
    reg = -1;
  }
}

std::string InterferenceGraph::ToString() const {
  std::stringstream ss;

  ss << "interference graph edges:";
  for (ir::value_num_t value : values_) {
    ss << "\n";
    ss << std::setw(4) << std::setfill(' ') << "%" << value << ": ";

    bool first = true;
    for (ir::value_num_t neighbor : graph_.at(value)) {
      if (first) {
        first = false;
      } else {
        ss << ", ";
      }
      ss << "%" << neighbor;
    }
  }
  ss << "\n\n";
  ss << "interference graph registers:";
  for (ir::value_num_t value : values_) {
    ss << "\n";
    ss << std::setw(4) << std::setfill(' ') << "%" << value << ": ";
    ss << std::setw(2) << std::setfill(' ') << regs_.at(value);
  }

  return ss.str();
}

vcg::Graph InterferenceGraph::ToVCGGraph() const {
  vcg::Graph vcg_graph;

  std::unordered_map<ir::value_num_t, int64_t> value_numbers;
  value_numbers.reserve(graph_.size());

  for (auto& [node, neighbors] : graph_) {
    int64_t node_number = value_numbers.size();
    int64_t node_reg = regs_.at(node);

    value_numbers.insert({node, node_number});

    vcg_graph.nodes().push_back(
        vcg::Node(node_number, std::to_string(node), "", (vcg::Color)node_reg));

    for (ir::value_num_t neighbor : neighbors) {
      auto it = value_numbers.find(neighbor);
      if (it == value_numbers.end()) continue;

      int64_t neighbor_number = it->second;

      vcg_graph.edges().push_back(vcg::Edge(node_number, neighbor_number));
    }
  }

  return vcg_graph;
}

}  // namespace ir_info
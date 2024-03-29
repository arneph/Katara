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
}

void InterferenceGraph::AddEdge(ir::value_num_t value_a, ir::value_num_t value_b) {
  auto it_a = graph_.find(value_a);
  if (it_a != graph_.end()) {
    it_a->second.insert(value_b);
  } else {
    values_.insert(value_a);
    graph_.insert({value_a, std::unordered_set<ir::value_num_t>{value_b}});
  }

  auto it_b = graph_.find(value_b);
  if (it_b != graph_.end()) {
    it_b->second.insert(value_a);
  } else {
    values_.insert(value_b);
    graph_.insert({value_b, std::unordered_set<ir::value_num_t>{value_a}});
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
    }
  }

  auto it = graph_.find(individual);
  if (it != graph_.end()) {
    it->second.insert(group.begin(), group.end());
  } else {
    values_.insert(individual);
    graph_.insert({individual, group});
  }
}

std::string InterferenceGraph::ToString() const {
  std::stringstream ss;
  ss << "interference graph:";
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
  return ss.str();
}

common::graph::Graph InterferenceGraph::ToGraph(const InterferenceGraphColors* colors) const {
  common::graph::Graph vcg_graph(/*is_directed=*/false);

  std::unordered_map<ir::value_num_t, int64_t> value_numbers;
  value_numbers.reserve(graph_.size());

  for (auto& [node, neighbors] : graph_) {
    int64_t node_number = value_numbers.size();
    int64_t node_reg = (colors != nullptr) ? colors->GetColor(node) : 0;

    value_numbers.insert({node, node_number});

    vcg_graph.nodes().push_back(common::graph::NodeBuilder(node_number, std::to_string(node))
                                    .SetColor(common::graph::Color(node_reg))
                                    .Build());

    for (ir::value_num_t neighbor : neighbors) {
      auto it = value_numbers.find(neighbor);
      if (it == value_numbers.end()) continue;

      int64_t neighbor_number = it->second;

      vcg_graph.edges().push_back(common::graph::Edge(node_number, neighbor_number));
    }
  }

  return vcg_graph;
}

color_t InterferenceGraphColors::GetColor(ir::value_num_t value) const {
  if (auto it = colors_.find(value); it != colors_.end()) {
    return it->second;
  } else {
    return kNoColor;
  }
}

std::unordered_set<ir_info::color_t> InterferenceGraphColors::GetColors(
    const std::unordered_set<ir::value_num_t>& values) const {
  std::unordered_set<ir_info::color_t> colors;
  for (ir::value_num_t value : values) {
    colors.insert(GetColor(value));
  }
  return colors;
}

std::string InterferenceGraphColors::ToString() const {
  std::stringstream ss;
  ss << "interference graph colors:";
  for (auto [value, color] : colors_) {
    ss << "\n";
    ss << std::setw(4) << std::setfill(' ') << "%" << value << ": ";
    ss << std::setw(2) << std::setfill(' ') << color;
  }
  return ss.str();
}

}  // namespace ir_info

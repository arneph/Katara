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

InterferenceGraph::InterferenceGraph() {}
InterferenceGraph::~InterferenceGraph() {}

const std::unordered_set<ir::Computed>&
InterferenceGraph::values() const {
    return values_;
}

const std::unordered_set<ir::Computed>&
InterferenceGraph::GetNeighbors(ir::Computed value) const {
    return graph_.at(value);
}

void InterferenceGraph::AddValue(ir::Computed value) {
    auto it = values_.find(value);
    if (it != values_.end()) {
        return;
    }
    
    values_.insert(value);
    graph_.insert({value, std::unordered_set<ir::Computed>()});
    regs_.insert({value, -1});
}

void InterferenceGraph::AddEdge(ir::Computed value_a,
                                ir::Computed value_b) {
    auto it_a = graph_.find(value_a);
    if (it_a != graph_.end()) {
        it_a->second.insert(value_b);
    } else {
        values_.insert(value_a);
        graph_.insert({value_a,
                       std::unordered_set<ir::Computed>{value_b}});
        regs_.insert({value_a, -1});
    }
    
    auto it_b = graph_.find(value_b);
    if (it_b != graph_.end()) {
        it_b->second.insert(value_a);
    } else {
        values_.insert(value_b);
        graph_.insert({value_b,
                       std::unordered_set<ir::Computed>{value_a}});
        regs_.insert({value_b, -1});
    }
}

void InterferenceGraph::AddEdgesIn(
    std::unordered_set<ir::Computed> group) {
    if (group.size() == 0) return;
    if (group.size() == 1) {
        AddValue(*group.begin());
        return;
    }
    
    for (ir::Computed group_member : group) {
        auto it = graph_.find(group_member);
        if (it == graph_.end()) {
            values_.insert(group_member);
            auto result = graph_.insert(
                {group_member, std::unordered_set<ir::Computed>()});
            regs_.insert({group_member, -1});
            
            it = result.first;
        }
        
        std::unordered_set<ir::Computed>& neighbors = it->second;
        
        for (ir::Computed other : group) {
            if (group_member == other) continue;
            
            neighbors.insert(other);
        }
    }
}

void InterferenceGraph::AddEdgesBetween(
    std::unordered_set<ir::Computed> group,
    ir::Computed individual) {
    for (ir::Computed group_member : group) {
        auto it = graph_.find(group_member);
        if (it != graph_.end()) {
            it->second.insert(individual);
        } else {
            values_.insert(group_member);
            graph_.insert(
                {group_member,
                 std::unordered_set<ir::Computed>{individual}});
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

int64_t InterferenceGraph::GetRegister(ir::Computed value) const {
    return regs_.at(value);
}

void InterferenceGraph::SetRegister(ir::Computed value, int64_t reg) {
    regs_[value] = reg;
}

void InterferenceGraph::ResetRegisters() {
    for (auto& [value, reg] : regs_) {
        reg = -1;
    }
}

std::string InterferenceGraph::ToString() const {
    std::stringstream ss;
    
    ss << "interference graph edges:";
    for (ir::Computed value : ir::set_to_ordered_vec(values_)) {
        ss << "\n";
        ss << std::setw(4) << std::setfill(' ')
           << value.ToString() << ": ";
        
        ir::set_to_stream(graph_.at(value), ss);
    }
    ss << "\n\n";
    ss << "interference graph registers:";
    for (ir::Computed value : ir::set_to_ordered_vec(values_)) {
        ss << "\n";
        ss << std::setw(4) << std::setfill(' ')
           << value.ToString() << ": ";
        ss << std::setw(2) << std::setfill(' ')
           << regs_.at(value);
    }
    
    return ss.str();
}

vcg::Graph InterferenceGraph::ToVCGGraph() const {
    vcg::Graph vcg_graph;
    
    std::unordered_map<ir::Computed, int64_t> value_numbers;
    value_numbers.reserve(graph_.size());
    
    for (auto& [node, neighbors] : graph_) {
        int64_t node_number = value_numbers.size();
        int64_t node_reg = regs_.at(node);
        
        value_numbers.insert({node, node_number});
        
        vcg_graph.nodes().push_back(vcg::Node(node_number,
                                              node.ToString(), "",
                                              (vcg::Color) node_reg));
        
        for (ir::Computed neighbor : neighbors) {
            auto it = value_numbers.find(neighbor);
            if (it == value_numbers.end()) continue;
            
            int64_t neighbor_number = it->second;
            
            vcg_graph.edges().push_back(vcg::Edge(node_number,
                                                  neighbor_number));
        }
    }
    
    return vcg_graph;
}

}

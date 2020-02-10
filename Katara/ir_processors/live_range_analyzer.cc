//
//  live_range_analyzer.cc
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "live_range_analyzer.h"

#include "ir_info/block_live_range_info.h"

namespace ir_proc {

LiveRangeAnalyzer::LiveRangeAnalyzer(ir::Func *func)
    : func_(func), func_info_(func) {}

LiveRangeAnalyzer::~LiveRangeAnalyzer() {}

ir_info::FuncLiveRangeInfo& LiveRangeAnalyzer::func_info() {
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
    
    std::unordered_set<ir::Block *> queue;
    
    for (ir::Block *block : func_->blocks()) {
        ir_info::BlockLiveRangeInfo &block_info
            = func_info_.GetBlockLiveRangeInfo(block);
        
        BacktraceBlock(block, block_info);
        
        if (!block_info.entry_set().empty()) {
            queue.insert(block);
        }
    }
    
    while (!queue.empty()) {
        auto it = queue.begin();
        ir::Block *block = *it;
        queue.erase(it);
        
        ir_info::BlockLiveRangeInfo& block_info =
            func_info_.GetBlockLiveRangeInfo(block);
        
        for (ir::Block *parent : block->parents()) {
            ir_info::BlockLiveRangeInfo& parent_info =
                func_info_.GetBlockLiveRangeInfo(parent);
            
            bool expanded_entry_set = false;
            
            for (ir::Computed value : block_info.entry_set()) {
                auto it_a = parent_info.exit_set().find(value);
                if (it_a != parent_info.exit_set().end()) {
                    continue;
                } else {
                    parent_info.exit_set().insert(value);
                }
                
                auto it_b = parent_info.definitions().find(value);
                if (it_b == parent_info.definitions().end()) {
                    parent_info.entry_set().insert(value);
                    expanded_entry_set = true;
                }
            }
            
            if (expanded_entry_set) {
                queue.insert(parent);
            }
        }
    }
}

void LiveRangeAnalyzer::BacktraceBlock(
    ir::Block *block,
    ir_info::BlockLiveRangeInfo &info) {
    
    // Add values defined in block to definitions
    for (ir::Instr *instr : block->instrs()) {
        for (ir::Computed defined_value : instr->DefinedValues()) {
            info.definitions().insert(defined_value);
        }
    }
    
    // Add values used in phi instructions of child to exit set
    // Also add values not defined in block to entry set
    if (block->HasMergingChild()) {
        block->MergingChild()->for_each_phi_instr(
            [&] (ir::PhiInstr *instr) {

            ir::Value value =
                instr->ValueInheritedFromBlock(block->number());
            if (!value.is_computed()) return;
            ir::Computed computed = value.computed();
            
            info.exit_set().insert(computed);
            
            auto it = info.definitions().find(computed);
            if (it == info.definitions().end()) {
                info.entry_set().insert(computed);
            }
        });
    }
    
    // Add values used, but not defined in block to entry set
    // excluding values used in phi instructions
    block->for_each_non_phi_instr([&] (ir::Instr *instr) {
        for (ir::Computed used_value : instr->UsedValues()) {
            auto it = info.definitions().find(used_value);
            if (it == info.definitions().end()) {
                info.entry_set().insert(used_value);
            }
        }
    });
}

void LiveRangeAnalyzer::BuildInterferenceGraph() {
    if (interference_graph_ok_) return;
    interference_graph_ok_ = true;
    
    FindLiveRanges();
    
    for (ir::Block *block : func_->blocks()) {
        ir_info::BlockLiveRangeInfo &block_info
            = func_info_.GetBlockLiveRangeInfo(block);
        
        BuildInterferenceGraph(block, block_info);
    }
}

void LiveRangeAnalyzer::BuildInterferenceGraph(
    ir::Block *block,
    ir_info::BlockLiveRangeInfo &info) {
    const size_t n = block->instrs().size();
    std::unordered_set<ir::Computed> live_set = info.exit_set();
    
    interference_graph_.AddEdgesIn(live_set);
    
    for (int64_t i = n - 1; i >= 0; i--) {
        ir::Instr *instr = block->instrs().at(i);
        bool is_phi =
            (dynamic_cast<ir::PhiInstr *>(instr) != nullptr);
        
        for (ir::Computed defined_value : instr->DefinedValues()) {
            auto it = live_set.find(defined_value);
            if (it == live_set.end()) {
                interference_graph_.AddEdgesBetween(live_set,
                                                    defined_value);
                
            } else {
                live_set.erase(it);
            }
        }
        
        for (ir::Computed used_value : instr->UsedValues()) {
            auto it = live_set.find(used_value);
            if (it == live_set.end()) {
                interference_graph_.AddEdgesBetween(live_set,
                                                    used_value);
                
                if (!is_phi) {
                    live_set.insert(used_value);
                }
            }
        }
    }
}

}

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
        
        if (!block_info.GetEntrySet().empty()) {
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
            
            size_t old_entry_set_size =
                parent_info.GetEntrySet().size();
            
            for (ir::Computed value : block_info.GetEntrySet()) {
                parent_info.PropagateBackwardsFromExitSet(value);
            }
            
            size_t new_entry_set_size =
                parent_info.GetEntrySet().size();
            
            if (old_entry_set_size < new_entry_set_size) {
                queue.insert(parent);
            }
        }
    }
}

void LiveRangeAnalyzer::BacktraceBlock(
    ir::Block *block,
    ir_info::BlockLiveRangeInfo &info) {
    const size_t n = block->instrs().size();
    
    // Backtrace through instructions in block
    // Add value defintions and uses (outside phi instructions)
    for (int64_t index = n - 1; index >= 0; index--) {
        ir::Instr *instr = block->instrs().at(index);
        
        for (ir::Computed defined_value : instr->DefinedValues()) {
            info.AddValueDefinition(defined_value, index);
        }
        
        if (dynamic_cast<ir::PhiInstr *>(instr) != nullptr) {
            continue;
        }
        for (ir::Computed used_value : instr->UsedValues()) {
            info.AddValueUse(used_value, index);
        }
    }
    
    // Include values used in phi instructions of child
    if (block->HasMergingChild()) {
        block->MergingChild()->for_each_phi_instr(
            [&] (ir::PhiInstr *instr) {
            ir::Value value =
                instr->ValueInheritedFromBlock(block->number());
            if (!value.is_computed()) return;
            ir::Computed computed = value.computed();
            
            info.PropagateBackwardsFromExitSet(computed);
        });
    }
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
    std::unordered_set<ir::Computed> live_set = info.GetExitSet();
    
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

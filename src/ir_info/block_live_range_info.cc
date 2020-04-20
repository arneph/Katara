//
//  block_live_range_info.cc
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "block_live_range_info.h"

#include <iomanip>
#include <sstream>

namespace ir_info {

BlockLiveRangeInfo::BlockLiveRangeInfo(ir::Block *block)
    : block_(block) {}

BlockLiveRangeInfo::~BlockLiveRangeInfo() {}

bool BlockLiveRangeInfo::HasValue(ir::Computed value) const {
    return value_ranges_.count(value) != 0;
}

bool BlockLiveRangeInfo::HasValueDefinition(ir::Computed value) const {
    if (value_ranges_.count(value) == 0) {
        return false;
    }
    return value_ranges_.at(value).start_index_ >= 0;
}

void BlockLiveRangeInfo::AddValueDefinition(ir::Computed value,
                                            int64_t index) {
    if (auto it = value_ranges_.find(value);
        it != value_ranges_.end()) {
        it->second.start_index_ = index;
    
    } else {
        value_ranges_.insert({value,
                             ValueRange{.start_index_ = index,
                                       .end_index_ = index}});
    }
}

void BlockLiveRangeInfo::AddValueUse(ir::Computed value,
                                     int64_t index) {
    if (auto it = value_ranges_.find(value);
        it != value_ranges_.end()) {
        it->second.start_index_ = std::min(it->second.start_index_,
                                           index);
        it->second.end_index_ = std::max(it->second.end_index_,
                                         index);
    } else {
        value_ranges_.insert({value,
                             ValueRange{.start_index_ = -1,
                                       .end_index_ = index}});
    }
}

void BlockLiveRangeInfo::PropagateBackwardsFromExitSet(
    ir::Computed value) {
    const int64_t exit_index = block_->instrs().size();
    
    if (auto it = value_ranges_.find(value);
        it != value_ranges_.end()) {
        it->second.end_index_ = exit_index;
        
    } else {
        value_ranges_.insert({value,
                             ValueRange{.start_index_ = -1,
                                       .end_index_ = exit_index}});
    }
}

std::unordered_set<ir::Computed>
BlockLiveRangeInfo::GetEntrySet() const {
    std::unordered_set<ir::Computed> entry_set;
    for (auto& [value, range] : value_ranges_) {
        if (range.start_index_ < 0) {
            entry_set.insert(value);
        }
    }
    return entry_set;
}

std::unordered_set<ir::Computed>
BlockLiveRangeInfo::GetExitSet() const {
    std::unordered_set<ir::Computed> exit_set;
    for (auto& [value, range] : value_ranges_) {
        if (range.end_index_ >= block_->instrs().size()) {
            exit_set.insert(value);
        }
    }
    return exit_set;
}

std::unordered_set<ir::Computed>
BlockLiveRangeInfo::GetLiveSet(int64_t index) const {
    std::unordered_set<ir::Computed> live_set;
    for (auto& [value, range] : value_ranges_) {
        if (range.start_index_ <= index && index <= range.end_index_) {
            live_set.insert(value);
        }
    }
    return live_set;
}

std::string BlockLiveRangeInfo::ToString() const {
    std::stringstream ss;
    
    ss << std::setw(5) << std::setfill(' ')
       << block_->ReferenceString()
       << " - live range info:\n";
    for (auto& [value, range] : value_ranges_) {
        if (range.start_index_ < 0) {
            ss << '<';
        } else {
            ss << ' ';
        }
        for (int i = 0; i < block_->instrs().size(); i++) {
            if (range.start_index_ < i && i < range.end_index_) {
                ss << '-';
            } else if (i == range.start_index_ ||
                       i == range.end_index_) {
                ss << '+';
            } else {
                ss << ' ';
            }
        }
        if (range.end_index_ == block_->instrs().size()) {
            ss << '>';
        } else {
            ss << ' ';
        }
        
        ss << ' ' << value.ToString() << '\n';
    }
    
    ss << "entry set: ";
    ir::set_to_stream(GetEntrySet(), ss);
    ss << '\n';
    
    ss << " exit set: ";
    ir::set_to_stream(GetExitSet(), ss);
    ss << '\n';
    
    return ss.str();
}

}

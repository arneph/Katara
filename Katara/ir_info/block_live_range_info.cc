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

std::unordered_set<ir::Computed>& BlockLiveRangeInfo::definitions() {
    return definitions_;
}

std::unordered_set<ir::Computed>& BlockLiveRangeInfo::entry_set() {
    return entry_set_;
}

std::unordered_set<ir::Computed>& BlockLiveRangeInfo::exit_set() {
    return exit_set_;
}

std::string BlockLiveRangeInfo::ToString() const {
    std::stringstream ss;
    
    ss << std::setw(5) << std::setfill(' ')
       << block_->ReferenceString()
       << " - entry live set: ";
    ir::set_to_stream(entry_set_, ss);
    ss << "\n";
    
    ss << std::setw(5) << std::setfill(' ')
       << block_->ReferenceString()
       << " -  exit live set: ";
    ir::set_to_stream(exit_set_, ss);
    
    return ss.str();
}

}

//
//  func_live_range_info.cc
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "func_live_range_info.h"

#include <sstream>

namespace ir_info {

FuncLiveRangeInfo::FuncLiveRangeInfo(ir::Func *func)
    : func_(func) {
    for (ir::Block *block : func_->blocks()) {
        block_live_range_infos_.insert({block,
                                        BlockLiveRangeInfo(block)});
    }
}

FuncLiveRangeInfo::~FuncLiveRangeInfo() {}

BlockLiveRangeInfo&
FuncLiveRangeInfo::GetBlockLiveRangeInfo(ir::Block *block) {
    return block_live_range_infos_.at(block);
}

std::string FuncLiveRangeInfo::ToString() const {
    std::stringstream ss;
    ss << "live range info for " << func_->ReferenceString() << ":";
    for (auto& [block, block_info] : block_live_range_infos_) {
        ss << "\n" << block_info.ToString();
    }
    return ss.str();
}

}

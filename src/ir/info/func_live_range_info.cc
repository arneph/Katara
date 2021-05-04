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

FuncLiveRangeInfo::FuncLiveRangeInfo(ir::Func* func) : func_(func) {
  for (auto& block : func->blocks()) {
    block_live_range_infos_.insert({block->number(), BlockLiveRangeInfo(block.get())});
  }
}

BlockLiveRangeInfo& FuncLiveRangeInfo::GetBlockLiveRangeInfo(ir::block_num_t bnum) {
  return block_live_range_infos_.at(bnum);
}

std::string FuncLiveRangeInfo::ToString() const {
  std::stringstream ss;
  ss << "live range info for " << func_->ReferenceString() << ":";
  for (auto& [bnum, block_info] : block_live_range_infos_) {
    ss << "\n" << block_info.ToString();
  }
  return ss.str();
}

}  // namespace ir_info

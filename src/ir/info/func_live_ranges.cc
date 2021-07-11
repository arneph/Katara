//
//  func_live_ranges.cc
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "func_live_ranges.h"

#include <sstream>

namespace ir_info {

FuncLiveRanges::FuncLiveRanges(const ir::Func* func) : func_(func) {
  for (auto& block : func->blocks()) {
    block_live_ranges_.insert({block->number(), BlockLiveRanges(block.get())});
  }
}

BlockLiveRanges& FuncLiveRanges::GetBlockLiveRanges(ir::block_num_t bnum) {
  return block_live_ranges_.at(bnum);
}

std::string FuncLiveRanges::ToString() const {
  std::stringstream ss;
  ss << "live ranges for " << func_->ReferenceString() << ":";
  for (auto& [bnum, block_info] : block_live_ranges_) {
    ss << "\n" << block_info.ToString();
  }
  return ss.str();
}

}  // namespace ir_info

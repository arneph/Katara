//
//  func.cc
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "func.h"

#include <sstream>

namespace x86_64 {

int64_t Func::Encode(Linker& linker, common::data code) const {
  linker.AddFuncAddr(func_id_, code.base());

  int64_t c = 0;
  for (auto& block : blocks_) {
    int64_t r = block->Encode(linker, code.view(c));
    if (r == -1) return -1;
    c += r;
  }
  return c;
}

std::string Func::ToString() const {
  std::stringstream ss;
  ss << name_ << ":\n";
  for (size_t i = 0; i < blocks_.size(); i++) {
    ss << blocks_[i]->ToString();
    if (i < blocks_.size() - 1) ss << "\n";
  }
  return ss.str();
}

BlockBuilder FuncBuilder::AddBlock() {
  int64_t block_id = block_count_++;
  Block* block = func_->blocks_.emplace_back(new Block(func_, block_id)).get();
  return BlockBuilder(block);
}

}  // namespace x86_64

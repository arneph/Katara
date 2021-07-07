//
//  block.cc
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "block.h"

#include <sstream>

#include "func.h"

namespace x86_64 {

int64_t Block::Encode(Linker& linker, common::data code) const {
  linker.AddBlockAddr(block_id_, code.base());

  int64_t c = 0;
  for (auto& instr : instrs_) {
    int8_t r = instr->Encode(linker, code.view(c));
    if (r == -1) return -1;
    c += r;
  }
  return c;
}

std::string Block::ToString() const {
  std::stringstream ss;
  ss << "BB" << std::to_string(block_id_) << ":\n";
  for (size_t i = 0; i < instrs_.size(); i++) {
    ss << "\t" << instrs_[i]->ToString();
    if (i < instrs_.size() - 1) ss << "\n";
  }
  return ss.str();
}

}  // namespace x86_64

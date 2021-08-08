//
//  block.cc
//  Katara
//
//  Created by Arne Philipeit on 11/27/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "block.h"

#include <sstream>

#include "src/x86_64/func.h"
#include "src/x86_64/program.h"

namespace x86_64 {

Program* Block::program() const { return func_->program(); }

int64_t Block::Encode(Linker& linker, common::DataView code) const {
  linker.AddBlockAddr(block_id_, code.base());

  int64_t code_index = 0;
  for (auto& instr : instrs_) {
    int8_t written_bytes = instr->Encode(linker, code.SubView(code_index));
    if (written_bytes == -1) return -1;
    code_index += written_bytes;
  }
  return code_index;
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

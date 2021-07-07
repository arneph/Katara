//
//  prog.cc
//  Katara
//
//  Created by Arne Philipeit on 12/7/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "program.h"

#include <sstream>

namespace x86_64 {

int64_t Program::Encode(Linker& linker, common::data code) const {
  int64_t c = 0;
  for (auto& func : funcs_) {
    int64_t r = func->Encode(linker, code.view(c));
    if (r == -1) return -1;
    c += r;
  }
  return c;
}

std::string Program::ToString() const {
  std::stringstream ss;
  for (size_t i = 0; i < funcs_.size(); i++) {
    ss << funcs_[i]->ToString();
    if (i < funcs_.size() - 1) ss << "\n\n";
  }
  return ss.str();
}

FuncBuilder ProgramBuilder::AddFunc(std::string func_name) {
  int64_t func_id = func_count_++;
  Func* func = program_->funcs_.emplace_back(new Func(program_.get(), func_id, func_name)).get();
  return FuncBuilder(func, block_count_);
}

}  // namespace x86_64

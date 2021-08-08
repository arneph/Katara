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

Func* Program::DefinedFuncWithName(std::string name) const {
  for (auto& func : defined_funcs_) {
    if (func->name() == name) {
      return func.get();
    }
  }
  return nullptr;
}

int64_t Program::Encode(Linker& linker, common::DataView code) const {
  int64_t c = 0;
  for (auto& func : defined_funcs_) {
    int64_t r = func->Encode(linker, code.SubView(c));
    if (r == -1) return -1;
    c += r;
  }
  return c;
}

std::string Program::ToString() const {
  std::stringstream ss;
  for (size_t i = 0; i < defined_funcs_.size(); i++) {
    ss << defined_funcs_[i]->ToString();
    if (i < defined_funcs_.size() - 1) ss << "\n\n";
  }
  return ss.str();
}

FuncBuilder ProgramBuilder::DefineFunc(std::string func_name) {
  int64_t func_id = func_count_++;
  Func* func =
      program_->defined_funcs_.emplace_back(new Func(program_.get(), func_id, func_name)).get();
  return FuncBuilder(func, block_count_);
}

void ProgramBuilder::DeclareFunc(std::string func_name) {
  int64_t func_id = func_count_++;
  program_->declared_funcs_.emplace(func_name, func_id);
}

}  // namespace x86_64

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

void Program::DeclareFunc(std::string func_name) {
  func_num_t func_num = defined_funcs_.size() + declared_funcs_.size();
  declared_funcs_.emplace(func_name, func_num);
}

Func* Program::DefineFunc(std::string func_name) {
  func_num_t func_num = defined_funcs_.size() + declared_funcs_.size();
  return defined_funcs_.emplace_back(new Func(this, func_num, func_name)).get();
}

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

}  // namespace x86_64

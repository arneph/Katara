//
//  prog.cc
//  Katara
//
//  Created by Arne Philipeit on 12/7/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "prog.h"

#include <sstream>

namespace x86_64 {

Prog::Prog() {}
Prog::~Prog() {}

const std::vector<Func*> Prog::funcs() const { return funcs_; }

int64_t Prog::Encode(Linker* linker, common::data code) const {
  int64_t c = 0;
  for (auto& func : funcs_) {
    int64_t r = func->Encode(linker, code.view(c));
    if (r == -1) return -1;
    c += r;
  }
  return c;
}

std::string Prog::ToString() const {
  std::stringstream ss;
  for (size_t i = 0; i < funcs_.size(); i++) {
    ss << funcs_[i]->ToString();
    if (i < funcs_.size() - 1) ss << "\n\n";
  }
  return ss.str();
}

ProgBuilder::ProgBuilder() {
  prog_ = new Prog();
  block_count_ = 0;
}

ProgBuilder::~ProgBuilder() {}

FuncBuilder ProgBuilder::AddFunc(std::string func_name) {
  FuncBuilder func_builder(prog_, func_count_++, func_name, block_count_);

  prog_->funcs_.push_back(func_builder.func());
  return func_builder;
}

Prog* ProgBuilder::prog() const { return prog_; }

}  // namespace x86_64

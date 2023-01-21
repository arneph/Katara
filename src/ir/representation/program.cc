//
//  prog.cc
//  Katara
//
//  Created by Arne Philipeit on 12/25/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "program.h"

#include <sstream>

#include "src/common/logging/logging.h"

namespace ir {

using ::common::logging::fail;

Func* Program::GetFunc(func_num_t fnum) const {
  auto it = std::find_if(funcs_.begin(), funcs_.end(),
                         [=](auto& func) { return func->number() == fnum; });
  return (it != funcs_.end()) ? it->get() : nullptr;
}

Func* Program::AddFunc(func_num_t fnum) {
  if (fnum == kNoFuncNum) {
    fnum = func_count_++;
  } else if (fnum < func_count_ && HasFunc(fnum)) {
    fail("tried to add function with used function number");
  } else {
    func_count_ = std::max(func_count_, fnum + 1);
  }
  auto func = std::make_unique<Func>(fnum);
  auto func_ptr = func.get();
  funcs_.push_back(std::move(func));
  return func_ptr;
}

void Program::RemoveFunc(func_num_t fnum) {
  auto it = std::find_if(funcs_.begin(), funcs_.end(),
                         [=](auto& func) { return func->number() == fnum; });
  if (it == funcs_.end()) fail("tried to remove func not owned by program");
  if (entry_func_num_ == fnum) entry_func_num_ = kNoFuncNum;
  funcs_.erase(it);
}

bool Program::operator==(const Program& that) const {
  if (entry_func_num() != that.entry_func_num()) return false;
  if (funcs().size() != that.funcs().size()) return false;
  for (std::size_t i = 0; i < funcs().size(); i++) {
    const Func* func_a = funcs().at(i).get();
    const Func* func_b = that.funcs().at(i).get();
    if (!IsEqual(func_a, func_b)) return false;
  }
  return true;
}

}  // namespace ir

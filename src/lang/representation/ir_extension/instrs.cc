//
//  instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 5/13/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "instrs.h"

namespace lang {
namespace ir_ext {

std::string StringConcatInstr::ToString() const {
  std::string str =
      result()->ToStringWithType() + " = concat:" + operands_.front()->type()->ToString() + " ";
  bool first = true;
  for (auto& operand : operands_) {
    if (first) {
      first = false;
    } else {
      str += ", ";
    }
    str + operand->ToString();
  }
  return str;
}

bool IsRefCountUpdateString(std::string op_str) { return op_str == "rcinc" || op_str == "rcdec"; }

RefCountUpdate ToRefCountUpdate(std::string op_str) {
  if (op_str == "rcinc") return RefCountUpdate::kInc;
  if (op_str == "rcdec") return RefCountUpdate::kDec;
  throw "unexpected ref count update";
}

std::string ToString(RefCountUpdate op) {
  switch (op) {
    case RefCountUpdate::kInc:
      return "rcinc";
    case RefCountUpdate::kDec:
      return "rcdec";
  }
}

}  // namespace ir_ext
}  // namespace lang

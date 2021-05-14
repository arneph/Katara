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
  std::string str = result()->ToStringWithType() + " = concat:string ";
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

}  // namespace ir_ext
}  // namespace lang

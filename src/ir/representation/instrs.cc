//
//  instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "instrs.h"

#include <array>
#include <utility>

#include "src/common/logging/logging.h"

namespace ir {

bool Instr::IsControlFlowInstr() const {
  switch (instr_kind()) {
    case InstrKind::kJump:
    case InstrKind::kJumpCond:
    case InstrKind::kReturn:
    case InstrKind::kLangPanic:
      return true;
    default:
      return false;
  }
}

void Instr::WriteRefString(std::ostream& os) const {
  bool wrote_first_defined_value = false;
  for (std::shared_ptr<Computed>& defined_value : DefinedValues()) {
    if (wrote_first_defined_value) {
      os << ", ";
    } else {
      wrote_first_defined_value = true;
    }
    defined_value->WriteRefStringWithType(os);
  }
  if (wrote_first_defined_value) {
    os << " = ";
  }
  os << OperationString();
  bool wrote_first_used_value = false;
  for (std::shared_ptr<Value>& used_value : UsedValues()) {
    if (wrote_first_used_value) {
      os << ", ";
    } else {
      os << " ";
      wrote_first_used_value = true;
    }
    if (used_value->kind() == Value::Kind::kConstant) {
      used_value->WriteRefStringWithType(os);
    } else {
      used_value->WriteRefString(os);
    }
  }
}

std::shared_ptr<Value> PhiInstr::ValueInheritedFromBlock(block_num_t bnum) const {
  for (auto arg : args_) {
    if (arg->origin() == bnum) {
      return arg->value();
    }
  }
  common::fail("phi instr does not inherit from block");
}

std::vector<std::shared_ptr<Value>> PhiInstr::UsedValues() const {
  std::vector<std::shared_ptr<Value>> used_values;
  for (auto arg : args_) {
    used_values.push_back(arg->value());
  }
  return used_values;
}

void PhiInstr::WriteRefString(std::ostream& os) const {
  result()->WriteRefString(os);
  os << " = " << OperationString() << " ";
  for (size_t i = 0; i < args_.size(); i++) {
    if (i > 0) os << ", ";
    args_.at(i)->WriteRefString(os);
  }
}

void JumpInstr::WriteRefString(std::ostream& os) const {
  os << OperationString() << " "
     << "{" << destination_ << "}";
}

void JumpCondInstr::WriteRefString(std::ostream& os) const {
  os << OperationString() << " "
     << "{" << destination_true_ << "}, {" << destination_false_ << "}";
}

std::vector<std::shared_ptr<Value>> CallInstr::UsedValues() const {
  std::vector<std::shared_ptr<Value>> used_values{func_};
  used_values.insert(used_values.end(), args_.begin(), args_.end());
  return used_values;
}

}  // namespace ir

//
//  instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "instrs.h"

#include "src/common/logging/logging.h"

namespace ir {

bool Instr::IsControlFlowInstr() const {
  switch (instr_kind()) {
    case InstrKind::kJump:
    case InstrKind::kJumpCond:
    case InstrKind::kReturn:
      return true;
    default:
      return false;
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

std::string PhiInstr::ToString() const {
  std::string str = result()->ToStringWithType() + " = phi ";
  for (size_t i = 0; i < args_.size(); i++) {
    if (i > 0) str += ", ";
    str += args_.at(i)->ToString();
  }
  return str;
}

std::string Conversion::ToString() const {
  return result()->ToStringWithType() + " = conv " + operand_->ToStringWithType();
}

std::string BoolNotInstr::ToString() const {
  return result()->ToStringWithType() + " = bnot " + operand_->ToString();
}

std::string BoolBinaryInstr::ToString() const {
  return result()->ToStringWithType() + " = " + common::ToString(operation_) + " " +
         operand_a_->ToString() + ", " + operand_b_->ToString();
}

std::string IntUnaryInstr::ToString() const {
  return result()->ToStringWithType() + " = " + common::ToString(operation_) + " " +
         operand_->ToString();
}

std::string IntCompareInstr::ToString() const {
  return result()->ToStringWithType() + " = " + common::ToString(operation_) + " " +
         operand_a_->ToString() + ", " + operand_b_->ToString();
}

std::string IntBinaryInstr::ToString() const {
  return result()->ToStringWithType() + " = " + common::ToString(operation_) + " " +
         operand_a_->ToString() + ", " + operand_b_->ToString();
}

std::string IntShiftInstr::ToString() const {
  return result()->ToStringWithType() + " = " + common::ToString(operation_) + " " +
         shifted_->ToString() + ", " + offset_->ToString();
}

std::string PointerOffsetInstr::ToString() const {
  return result()->ToStringWithType() + " = poff " + pointer_->ToString() + ", " +
         offset_->ToString();
}

std::string JumpCondInstr::ToString() const {
  return "jcc " + condition_->ToString() + ", {" + std::to_string(destination_true_) + "}, {" +
         std::to_string(destination_false_) + "}";
}

std::vector<std::shared_ptr<Value>> CallInstr::UsedValues() const {
  std::vector<std::shared_ptr<Value>> used_values{func_};
  used_values.insert(used_values.end(), args_.begin(), args_.end());
  return used_values;
}

std::string CallInstr::ToString() const {
  std::string str = "";
  for (size_t i = 0; i < results_.size(); i++) {
    if (i > 0) str += ", ";
    str += results_.at(i)->ToStringWithType();
  }
  if (results_.size() > 0) str += " = ";
  str += "call " + func_->ToString();
  for (size_t i = 0; i < args_.size(); i++) {
    str += ", " + args_.at(i)->ToString();
  }
  return str;
}

std::string ReturnInstr::ToString() const {
  std::string str = "ret";
  for (size_t i = 0; i < args_.size(); i++) {
    if (i == 0)
      str += " ";
    else
      str += ", ";
    str += args_.at(i)->ToStringWithType();
  }
  return str;
}

}  // namespace ir

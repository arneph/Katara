//
//  instrs.cc
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "instrs.h"

namespace ir {

MovInstr::MovInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> origin)
    : Computation(result), origin_(origin) {
  if (result->type() != origin->type()) {
    throw "attempted to create mov instr with mismatched origin type";
  }
}

PhiInstr::PhiInstr(std::shared_ptr<Computed> result,
                   std::vector<std::shared_ptr<InheritedValue>> args)
    : Computation(result), args_(args) {
  for (auto arg : args) {
    if (result->type() != arg->type()) {
      throw "attempted to create phi instr with mismatched arg type";
    }
  }
}

std::shared_ptr<Value> PhiInstr::ValueInheritedFromBlock(block_num_t bnum) const {
  for (auto arg : args_) {
    if (arg->origin() == bnum) {
      return arg->value();
    }
  }
  throw "phi instr does not inherit from block";
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

Conversion::Conversion(std::shared_ptr<Computed> result, std::shared_ptr<Value> operand)
    : Computation(result), operand_(operand) {
  switch (result->type()->type_kind()) {
    case TypeKind::kBool:
    case TypeKind::kInt:
    case TypeKind::kPointer:
    case TypeKind::kFunc:
      break;
    default:
      throw "internal error: result of conversion instr does not have expected type";
  }
  switch (operand->type()->type_kind()) {
    case TypeKind::kBool:
    case TypeKind::kInt:
    case TypeKind::kPointer:
    case TypeKind::kFunc:
      break;
    default:
      throw "internal error: operand of conversion instr does not have expected type";
  }
}

std::string Conversion::ToString() const {
  return result()->ToStringWithType() + " = conv " + operand_->ToStringWithType();
}

BoolNotInstr::BoolNotInstr(std::shared_ptr<Computed> result, std::shared_ptr<Value> operand)
    : Computation(result), operand_(operand) {
  if (result->type() != &kBool) {
    throw "internal error: result of bool not instr is not of type bool";
  } else if (operand->type() != &kBool) {
    throw "internal error: operand of bool not instr is not of type bool";
  }
}

std::string BoolNotInstr::ToString() const {
  return result()->ToStringWithType() + " = not " + operand_->ToString();
}

BoolBinaryInstr::BoolBinaryInstr(std::shared_ptr<Computed> result, common::Bool::BinaryOp operation,
                                 std::shared_ptr<Value> operand_a, std::shared_ptr<Value> operand_b)
    : Computation(result), operation_(operation), operand_a_(operand_a), operand_b_(operand_b) {
  if (result->type() != &kBool) {
    throw "internal error: result of bool binary instr is not of type bool";
  } else if (operand_a->type() != &kBool || operand_b->type() != &kBool) {
    throw "internal error: operand of bool binary instr is not of type bool";
  }
}

std::string BoolBinaryInstr::ToString() const {
  return result()->ToStringWithType() + " = " + common::ToString(operation_) + " " +
         operand_a_->ToString() + ", " + operand_b_->ToString();
}

IntUnaryInstr::IntUnaryInstr(std::shared_ptr<Computed> result, common::Int::UnaryOp operation,
                             std::shared_ptr<Value> operand)
    : Computation(result), operation_(operation), operand_(operand) {
  if (result->type()->type_kind() != TypeKind::kInt) {
    throw "internal error: result of int unary instr is not of type int";
  } else if (operand->type()->type_kind() != TypeKind::kInt) {
    throw "internal error: operand of int unary instr is not of type int";
  } else if (result->type() != operand->type()) {
    throw "internal error: result and operand of int unary instr have different types";
  }
}

std::string IntUnaryInstr::ToString() const {
  return result()->ToStringWithType() + " = " + common::ToString(operation_) + " " +
         operand_->ToString();
}

IntCompareInstr::IntCompareInstr(std::shared_ptr<Computed> result, common::Int::CompareOp operation,
                                 std::shared_ptr<Value> operand_a, std::shared_ptr<Value> operand_b)
    : Computation(result), operation_(operation), operand_a_(operand_a), operand_b_(operand_b) {
  if (result->type() != &kBool) {
    throw "internal error: result of int compare instr is not of type bool";
  } else if (operand_a->type()->type_kind() != TypeKind::kInt ||
             operand_b->type()->type_kind() != TypeKind::kInt) {
    throw "internal error: operand of int compare instr is not of type int";
  } else if (operand_a->type() != operand_b->type()) {
    throw "internal error: operands of int compare instr have different types";
  }
}

std::string IntCompareInstr::ToString() const {
  return result()->ToStringWithType() + " = " + common::ToString(operation_) + " " +
         operand_a_->ToString() + ", " + operand_b_->ToString();
}

IntBinaryInstr::IntBinaryInstr(std::shared_ptr<Computed> result, common::Int::BinaryOp operation,
                               std::shared_ptr<Value> operand_a, std::shared_ptr<Value> operand_b)
    : Computation(result), operation_(operation), operand_a_(operand_a), operand_b_(operand_b) {
  if (result->type()->type_kind() != TypeKind::kInt) {
    throw "internal error: result of int binary instr is not of type int";
  } else if (operand_a->type()->type_kind() != TypeKind::kInt ||
             operand_b->type()->type_kind() != TypeKind::kInt) {
    throw "internal error: operand of int binary instr is not of type int";
  } else if (result->type() != operand_a->type() || result->type() != operand_b->type()) {
    throw "internal error: result and operands of int binary instr have different types";
  }
}

std::string IntBinaryInstr::ToString() const {
  return result()->ToStringWithType() + " = " + common::ToString(operation_) + " " +
         operand_a_->ToString() + ", " + operand_b_->ToString();
}

IntShiftInstr::IntShiftInstr(std::shared_ptr<Computed> result, common::Int::ShiftOp operation,
                             std::shared_ptr<Value> shifted, std::shared_ptr<Value> offset)
    : Computation(result), operation_(operation), shifted_(shifted), offset_(offset) {
  if (result->type()->type_kind() != TypeKind::kInt) {
    throw "internal error: result of int shift instr is not of type int";
  } else if (shifted->type()->type_kind() != TypeKind::kInt ||
             offset->type()->type_kind() != TypeKind::kInt) {
    throw "internal error: operand of int shift instr is not of type int";
  } else if (result->type() != shifted->type()) {
    throw "internal error: result and shifted operand of int binary instr have different types";
  }
}

std::string IntShiftInstr::ToString() const {
  return result()->ToStringWithType() + " = " + common::ToString(operation_) + " " +
         shifted_->ToString() + ", " + offset_->ToString();
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

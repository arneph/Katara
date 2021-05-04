//
//  instr.cc
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "instr.h"

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

bool IsUnaryALOperationString(std::string op_str) { return op_str == "not" || op_str == "neg"; }

UnaryALOperation ToUnaryALOperation(std::string op_str) {
  if (op_str == "not") return UnaryALOperation::kNot;
  if (op_str == "neg") return UnaryALOperation::kNeg;
  throw "unknown unary al operation string";
}

std::string ToString(UnaryALOperation op) {
  switch (op) {
    case UnaryALOperation::kNot:
      return "not";
    case UnaryALOperation::kNeg:
      return "neg";
  }
}

UnaryALInstr::UnaryALInstr(UnaryALOperation operation, std::shared_ptr<Computed> result,
                           std::shared_ptr<Value> operand)
    : Computation(result), operation_(operation), operand_(operand) {
  if (result->type() != operand->type())
    throw "attempted to create unary al instr with mismatched operand type";
}

std::string UnaryALInstr::ToString() const {
  return result()->ToStringWithType() + " = " + ir::ToString(operation_) + ":" +
         operand_->type()->ToString() + " " + operand_->ToString();
}

bool IsBinaryALOperationString(std::string op_str) {
  return op_str == "and" || op_str == "or" || op_str == "xor" || op_str == "add" ||
         op_str == "sub" || op_str == "mul" || op_str == "div" || op_str == "rem";
}

BinaryALOperation ToBinaryALOperation(std::string op_str) {
  if (op_str == "and") return BinaryALOperation::kAnd;
  if (op_str == "or") return BinaryALOperation::kOr;
  if (op_str == "xor") return BinaryALOperation::kXor;
  if (op_str == "add") return BinaryALOperation::kAdd;
  if (op_str == "sub") return BinaryALOperation::kSub;
  if (op_str == "mul") return BinaryALOperation::kMul;
  if (op_str == "div") return BinaryALOperation::kDiv;
  if (op_str == "rem") return BinaryALOperation::kRem;
  throw "unknown binary al operation string";
}

std::string ToString(BinaryALOperation op) {
  switch (op) {
    case BinaryALOperation::kAnd:
      return "and";
    case BinaryALOperation::kOr:
      return "or";
    case BinaryALOperation::kXor:
      return "xor";
    case BinaryALOperation::kAdd:
      return "add";
    case BinaryALOperation::kSub:
      return "sub";
    case BinaryALOperation::kMul:
      return "mul";
    case BinaryALOperation::kDiv:
      return "div";
    case BinaryALOperation::kRem:
      return "rem";
  }
}

BinaryALInstr::BinaryALInstr(BinaryALOperation operation, std::shared_ptr<Computed> result,
                             std::shared_ptr<Value> operand_a, std::shared_ptr<Value> operand_b)
    : Computation(result), operation_(operation), operand_a_(operand_a), operand_b_(operand_b) {
  if (result->type() != operand_a->type() || result->type() != operand_b->type())
    throw "attempted to create binary al instr with mismatched operand type";
}

std::string BinaryALInstr::ToString() const {
  return result()->ToStringWithType() + " = " + ir::ToString(operation_) + ":" +
         operand_a_->type()->ToString() + " " + operand_a_->ToString() + ", " +
         operand_b_->ToString();
}

CompareOperation Comuted(CompareOperation op) {
  switch (op) {
    case CompareOperation::kEqual:
    case CompareOperation::kNotEqual:
      return op;

    case CompareOperation::kGreater:
      return CompareOperation::kLess;

    case CompareOperation::kGreaterOrEqual:
      return CompareOperation::kLessOrEqual;

    case CompareOperation::kLessOrEqual:
      return CompareOperation::kGreaterOrEqual;

    case CompareOperation::kLess:
      return CompareOperation::kGreater;
  }
}

CompareOperation Negated(CompareOperation op) {
  switch (op) {
    case CompareOperation::kEqual:
      return CompareOperation::kNotEqual;

    case CompareOperation::kNotEqual:
      return CompareOperation::kEqual;

    case CompareOperation::kGreater:
      return CompareOperation::kLessOrEqual;

    case CompareOperation::kGreaterOrEqual:
      return CompareOperation::kLess;

    case CompareOperation::kLessOrEqual:
      return CompareOperation::kGreater;

    case CompareOperation::kLess:
      return CompareOperation::kGreaterOrEqual;
  }
}

bool IsCompareOperationString(std::string op_str) {
  return op_str == "eq" || op_str == "ne" || op_str == "gt" || op_str == "gte" || op_str == "lte" ||
         op_str == "lt";
}

CompareOperation ToCompareOperation(std::string op_str) {
  if (op_str == "eq") return CompareOperation::kEqual;
  if (op_str == "ne") return CompareOperation::kNotEqual;
  if (op_str == "gt") return CompareOperation::kGreater;
  if (op_str == "gte") return CompareOperation::kGreaterOrEqual;
  if (op_str == "lte") return CompareOperation::kLessOrEqual;
  if (op_str == "lt") return CompareOperation::kLess;
  throw "unknown compare operation string";
}

std::string ToString(CompareOperation op) {
  switch (op) {
    case CompareOperation::kEqual:
      return "eq";
    case CompareOperation::kNotEqual:
      return "ne";
    case CompareOperation::kGreater:
      return "gt";
    case CompareOperation::kGreaterOrEqual:
      return "gte";
    case CompareOperation::kLessOrEqual:
      return "lte";
    case CompareOperation::kLess:
      return "lt";
  }
}

CompareInstr::CompareInstr(CompareOperation operation, std::shared_ptr<Computed> result,
                           std::shared_ptr<Value> operand_a, std::shared_ptr<Value> operand_b)
    : Computation(result), operation_(operation), operand_a_(operand_a), operand_b_(operand_b) {
  if (operand_a->type() != operand_b->type())
    throw "attempted to create compare instr with mismatched operand type";
}

std::string CompareInstr::ToString() const {
  return result()->ToStringWithType() + " = " + ir::ToString(operation_) + ":" +
         operand_a_->type()->ToString() + " " + operand_a_->ToString() + ", " +
         operand_b_->ToString();
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

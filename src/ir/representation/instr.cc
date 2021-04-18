//
//  instr.cc
//  Katara
//
//  Created by Arne Philipeit on 12/21/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "instr.h"

#include "ir/representation/block.h"

namespace ir {

Instr::Instr() {
  number_ = -1;
  block_ = nullptr;
}

int64_t Instr::number() const { return number_; }

Block* Instr::block() const { return block_; }

Computation::Computation(Computed result) : result_(result) {}

Computation::~Computation() {}

Computed Computation::result() const { return result_; }

std::vector<Computed> Computation::DefinedValues() const { return std::vector<Computed>{result_}; }

MovInstr::MovInstr(Computed result, Value origin) : Computation(result), origin_(origin) {
  if (result.type() != origin.type())
    throw "attempted to create mov instr with mismatched origin type";
}

MovInstr::~MovInstr() {}

Value MovInstr::origin() const { return origin_; }

std::vector<Computed> MovInstr::UsedValues() const {
  if (origin_.is_computed()) {
    return std::vector<Computed>{origin_.computed()};
  } else {
    return std::vector<Computed>();
  }
}

std::string MovInstr::ToString() const {
  return result().ToStringWithType() + " = mov " + origin().ToString();
}

PhiInstr::PhiInstr(Computed result, std::vector<InheritedValue> args)
    : Computation(result), args_(args) {
  for (InheritedValue arg : args) {
    if (result.type() != arg.type()) throw "attempted to create phi instr with mismatched arg type";
  }
}

PhiInstr::~PhiInstr() {}

std::vector<InheritedValue> const& PhiInstr::args() const { return args_; }

Value PhiInstr::ValueInheritedFromBlock(int64_t bnum) const {
  for (InheritedValue arg : args_) {
    if (arg.origin().block() == bnum) {
      return arg.value();
    }
  }
  throw "phi instr does not inherit from block";
}

std::vector<Computed> PhiInstr::UsedValues() const {
  std::vector<Computed> used_values;
  for (InheritedValue arg : args_) {
    if (arg.value().is_computed()) {
      used_values.push_back(arg.value().computed());
    }
  }
  return used_values;
}

std::string PhiInstr::ToString() const {
  std::string str = result().ToStringWithType() + " = phi ";
  for (size_t i = 0; i < args_.size(); i++) {
    if (i > 0) str += ", ";
    str += args_[i].ToString();
  }
  return str;
}

bool is_unary_al_operation_string(std::string op_str) { return op_str == "not" || op_str == "neg"; }

UnaryALOperation to_unary_al_operation(std::string op_str) {
  if (op_str == "not") return UnaryALOperation::kNot;
  if (op_str == "neg") return UnaryALOperation::kNeg;
  throw "unknown unary al operation string";
}

std::string to_string(UnaryALOperation op) {
  switch (op) {
    case UnaryALOperation::kNot:
      return "not";
    case UnaryALOperation::kNeg:
      return "neg";
  }
}

UnaryALInstr::UnaryALInstr(UnaryALOperation operation, Computed result, Value operand)
    : Computation(result), operation_(operation), operand_(operand) {
  if (!is_integral(result.type()))
    throw "attempted to create unary al instr with non-integral result type";
  if (result.type() != operand.type())
    throw "attempted to create unary al instr with mismatched operand type";
}

UnaryALInstr::~UnaryALInstr() {}

UnaryALOperation UnaryALInstr::operation() const { return operation_; }

Value UnaryALInstr::operand() const { return operand_; }

std::vector<Computed> UnaryALInstr::UsedValues() const {
  if (operand_.is_computed()) {
    return std::vector<Computed>{operand_.computed()};
  } else {
    return std::vector<Computed>();
  }
}

std::string UnaryALInstr::ToString() const {
  return result().ToStringWithType() + " = " + to_string(operation_) + ":" +
         to_string(operand_.type()) + " " + operand_.ToString();
}

bool is_binary_al_operation_string(std::string op_str) {
  return op_str == "and" || op_str == "or" || op_str == "xor" || op_str == "add" ||
         op_str == "sub" || op_str == "mul" || op_str == "div" || op_str == "rem";
}

BinaryALOperation to_binary_al_operation(std::string op_str) {
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

std::string to_string(BinaryALOperation op) {
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

BinaryALInstr::BinaryALInstr(BinaryALOperation operation, Computed result, Value operand_a,
                             Value operand_b)
    : Computation(result), operation_(operation), operand_a_(operand_a), operand_b_(operand_b) {
  if (!is_integral(result.type()))
    throw "attempted to create binary al instr with non-integral result type";
  if (result.type() != operand_a.type() || result.type() != operand_b.type())
    throw "attempted to create binary al instr with mismatched operand type";
}

BinaryALInstr::~BinaryALInstr() {}

BinaryALOperation BinaryALInstr::operation() const { return operation_; }

Value BinaryALInstr::operand_a() const { return operand_a_; }

Value BinaryALInstr::operand_b() const { return operand_b_; }

std::vector<Computed> BinaryALInstr::UsedValues() const {
  std::vector<Computed> used_values;
  if (operand_a_.is_computed()) {
    used_values.push_back(operand_a_.computed());
  }
  if (operand_b_.is_computed()) {
    used_values.push_back(operand_b_.computed());
  }
  return used_values;
}

std::string BinaryALInstr::ToString() const {
  return result().ToStringWithType() + " = " + to_string(operation_) + ":" +
         to_string(operand_a_.type()) + " " + operand_a_.ToString() + ", " + operand_b_.ToString();
}

CompareOperation comuted(CompareOperation op) {
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

CompareOperation negated(CompareOperation op) {
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

bool is_compare_operation_string(std::string op_str) {
  return op_str == "eq" || op_str == "ne" || op_str == "gt" || op_str == "gte" || op_str == "lte" ||
         op_str == "lt";
}

CompareOperation to_compare_operation(std::string op_str) {
  if (op_str == "eq") return CompareOperation::kEqual;
  if (op_str == "ne") return CompareOperation::kNotEqual;
  if (op_str == "gt") return CompareOperation::kGreater;
  if (op_str == "gte") return CompareOperation::kGreaterOrEqual;
  if (op_str == "lte") return CompareOperation::kLessOrEqual;
  if (op_str == "lt") return CompareOperation::kLess;
  throw "unknown compare operation string";
}

std::string to_string(CompareOperation op) {
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

CompareInstr::CompareInstr(CompareOperation operation, Computed result, Value operand_a,
                           Value operand_b)
    : Computation(result), operation_(operation), operand_a_(operand_a), operand_b_(operand_b) {
  if (result.type() != Type::kBool)
    throw "attempted to create compare instr with non-bool result type";
  if (!is_integral(operand_a.type()))
    throw "attempted to create compare instr with non-integral operand type";
  if (operand_a.type() != operand_b.type())
    throw "attempted to create compare instr with mismatched operand type";
}

CompareInstr::~CompareInstr() {}

CompareOperation CompareInstr::operation() const { return operation_; }

Value CompareInstr::operand_a() const { return operand_a_; }

Value CompareInstr::operand_b() const { return operand_b_; }

std::vector<Computed> CompareInstr::UsedValues() const {
  std::vector<Computed> used_values;
  if (operand_a_.is_computed()) {
    used_values.push_back(operand_a_.computed());
  }
  if (operand_b_.is_computed()) {
    used_values.push_back(operand_b_.computed());
  }
  return used_values;
}

std::string CompareInstr::ToString() const {
  return result().ToStringWithType() + " = " + to_string(operation_) + ":" +
         to_string(operand_a_.type()) + " " + operand_a_.ToString() + ", " + operand_b_.ToString();
}

JumpInstr::JumpInstr(BlockValue destionation) : destination_(destionation) {}

JumpInstr::~JumpInstr() {}

BlockValue JumpInstr::destination() const { return destination_; }

std::vector<Computed> JumpInstr::DefinedValues() const { return std::vector<Computed>(); }

std::vector<Computed> JumpInstr::UsedValues() const { return std::vector<Computed>(); }

std::string JumpInstr::ToString() const { return "jmp " + destination_.ToString(); }

JumpCondInstr::JumpCondInstr(Value condition, BlockValue destination_true,
                             BlockValue destination_false)
    : condition_(condition),
      destination_true_(destination_true),
      destination_false_(destination_false) {
  if (condition.type() != Type::kBool)
    throw "attempted to create jump cond instr with non-bool condition value";
}

JumpCondInstr::~JumpCondInstr() {}

Value JumpCondInstr::condition() const { return condition_; }

BlockValue JumpCondInstr::destination_true() const { return destination_true_; }

BlockValue JumpCondInstr::destination_false() const { return destination_false_; }

std::vector<Computed> JumpCondInstr::DefinedValues() const { return std::vector<Computed>(); }

std::vector<Computed> JumpCondInstr::UsedValues() const {
  if (condition_.is_computed()) {
    return std::vector<Computed>{condition_.computed()};
  } else {
    return std::vector<Computed>();
  }
}

std::string JumpCondInstr::ToString() const {
  return "jcc " + condition_.ToString() + ", " + destination_true_.ToString() + ", " +
         destination_false_.ToString();
}

CallInstr::CallInstr(Value func, std::vector<Computed> results, std::vector<Value> args)
    : func_(func), results_(results), args_(args) {
  if (func.type() != Type::kFunc)
    throw "attempted to create call instr with non-function func type";
}

CallInstr::~CallInstr() {}

Value CallInstr::func() const { return func_; }

const std::vector<Computed>& CallInstr::results() const { return results_; }

const std::vector<Value>& CallInstr::args() const { return args_; }

std::vector<Computed> CallInstr::DefinedValues() const { return results_; }

std::vector<Computed> CallInstr::UsedValues() const {
  std::vector<Computed> used_values;
  for (Value arg : args_) {
    if (arg.is_computed()) {
      used_values.push_back(arg.computed());
    }
  }
  return used_values;
}

std::string CallInstr::ToString() const {
  std::string str = "";
  for (size_t i = 0; i < results_.size(); i++) {
    if (i > 0) str += ", ";
    str += results_.at(i).ToStringWithType();
  }
  if (results_.size() > 0) str += " = ";
  str += "call " + func_.ToString();
  for (size_t i = 0; i < args_.size(); i++) {
    str += ", " + args_.at(i).ToStringWithType();
  }
  return str;
}

ReturnInstr::ReturnInstr(std::vector<Value> args) : args_(args) {}

ReturnInstr::~ReturnInstr() {}

const std::vector<Value>& ReturnInstr::args() const { return args_; }

std::vector<Computed> ReturnInstr::DefinedValues() const { return std::vector<Computed>(); }

std::vector<Computed> ReturnInstr::UsedValues() const {
  std::vector<Computed> used_values;
  for (Value arg : args_) {
    if (arg.is_computed()) {
      used_values.push_back(arg.computed());
    }
  }
  return used_values;
}

std::string ReturnInstr::ToString() const {
  std::string str = "ret";
  for (size_t i = 0; i < args_.size(); i++) {
    if (i == 0)
      str += " ";
    else
      str += ", ";
    str += args_.at(i).ToStringWithType();
  }
  return str;
}

}  // namespace ir

//
//  atomics.cc
//  Katara
//
//  Created by Arne Philipeit on 6/26/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "atomics.h"

namespace common {

std::string ToString(IntType type) {
  switch (type) {
    case IntType::kI8:
      return "i8";
    case IntType::kI16:
      return "i16";
    case IntType::kI32:
      return "i32";
    case IntType::kI64:
      return "i64";
    case IntType::kU8:
      return "u8";
    case IntType::kU16:
      return "u16";
    case IntType::kU32:
      return "u32";
    case IntType::kU64:
      return "u64";
  }
}

std::string Bool::ToString(bool a) { return a ? "true" : "false"; }

std::string ToString(Bool::BinaryOp op) {
  switch (op) {
    case Bool::BinaryOp::kEq:
      return "eq";
    case Bool::BinaryOp::kNeq:
      return "neq";
    case Bool::BinaryOp::kAnd:
      return "and";
    case Bool::BinaryOp::kOr:
      return "or";
  }
}

std::string Int::ToString() const {
  return std::visit([](auto&& value) { return std::to_string(value); }, value_);
}

std::string ToString(Int::UnaryOp op) {
  switch (op) {
    case Int::UnaryOp::kNeg:
      return "neg";
    case Int::UnaryOp::kNot:
      return "not";
  }
}

std::string ToString(Int::CompareOp op) {
  switch (op) {
    case Int::CompareOp::kEq:
      return "eq";
    case Int::CompareOp::kNeq:
      return "neq";
    case Int::CompareOp::kLss:
      return "lss";
    case Int::CompareOp::kLeq:
      return "leq";
    case Int::CompareOp::kGeq:
      return "geq";
    case Int::CompareOp::kGtr:
      return "gtr";
  }
}

std::string ToString(Int::BinaryOp op) {
  switch (op) {
    case Int::BinaryOp::kAdd:
      return "add";
    case Int::BinaryOp::kSub:
      return "sub";
    case Int::BinaryOp::kMul:
      return "mul";
    case Int::BinaryOp::kDiv:
      return "div";
    case Int::BinaryOp::kRem:
      return "rem";
    case Int::BinaryOp::kAnd:
      return "and";
    case Int::BinaryOp::kOr:
      return "or";
    case Int::BinaryOp::kXor:
      return "xor";
    case Int::BinaryOp::kAndNot:
      return "andnot";
  }
}

std::string ToString(Int::ShiftOp op) {
  switch (op) {
    case Int::ShiftOp::kLeft:
      return "shl";
    case Int::ShiftOp::kRight:
      return "shr";
  }
}

}  // namespace common

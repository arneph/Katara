//
//  atomics.cc
//  Katara
//
//  Created by Arne Philipeit on 6/26/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "atomics.h"

#include <errno.h>

#include <iomanip>
#include <sstream>

#include "src/common/logging/logging.h"

namespace common {

std::optional<IntType> ToIntType(std::string_view str) {
  if (str == "i8") {
    return IntType::kI8;
  } else if (str == "i16") {
    return IntType::kI16;
  } else if (str == "i32") {
    return IntType::kI32;
  } else if (str == "i64") {
    return IntType::kI64;
  } else if (str == "u8") {
    return IntType::kU8;
  } else if (str == "u16") {
    return IntType::kU16;
  } else if (str == "u32") {
    return IntType::kU32;
  } else if (str == "u64") {
    return IntType::kU64;
  } else {
    return std::nullopt;
  }
}

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

std::optional<Bool::BinaryOp> ToBoolBinaryOp(std::string_view str) {
  if (str == "beq") {
    return Bool::BinaryOp::kEq;
  } else if (str == "bneq") {
    return Bool::BinaryOp::kNeq;
  } else if (str == "band") {
    return Bool::BinaryOp::kAnd;
  } else if (str == "bor") {
    return Bool::BinaryOp::kOr;
  } else {
    return std::nullopt;
  }
}

std::string ToString(Bool::BinaryOp op) {
  switch (op) {
    case Bool::BinaryOp::kEq:
      return "beq";
    case Bool::BinaryOp::kNeq:
      return "bneq";
    case Bool::BinaryOp::kAnd:
      return "band";
    case Bool::BinaryOp::kOr:
      return "bor";
  }
}

std::string Int::ToString(Base base) const {
  std::stringstream ss;
  ss << std::setbase(base);
  std::visit([&ss](auto&& value) { ss << value; }, value_);
  return ss.str();
}

std::optional<Int> ToI64(std::string_view str, Int::Base base) {
  if (base == 1 || base > 36) {
    fail("unsupported integer base");
  } else if (str.length() == 0 || std::isspace(str.front())) {
    return std::nullopt;
  }
  const char* start = str.data();
  char* end;
  errno = 0;
  int64_t result = std::strtoll(start, &end, base);
  if (errno != 0 || end != start + str.length()) {
    errno = 0;
    return std::nullopt;
  }
  return common::Int(result);
}

std::optional<Int> ToU64(std::string_view str, Int::Base base) {
  if (base == 1 || base > 36) {
    fail("unsupported integer base");
  } else if (str.length() == 0 || std::isspace(str.front()) || str.front() == '-') {
    return std::nullopt;
  }
  const char* start = str.data();
  char* end;
  errno = 0;
  uint64_t result = std::strtoull(start, &end, base);
  if (errno != 0 || end != start + str.length()) {
    errno = 0;
    return std::nullopt;
  }
  return common::Int(result);
}

std::optional<Int::UnaryOp> ToIntUnaryOp(std::string_view str) {
  if (str == "ineg") {
    return Int::UnaryOp::kNeg;
  } else if (str == "inot") {
    return Int::UnaryOp::kNot;
  } else {
    return std::nullopt;
  }
}

std::string ToString(Int::UnaryOp op) {
  switch (op) {
    case Int::UnaryOp::kNeg:
      return "ineg";
    case Int::UnaryOp::kNot:
      return "inot";
  }
}

std::optional<Int::CompareOp> ToIntCompareOp(std::string_view str) {
  if (str == "ieq") {
    return Int::CompareOp::kEq;
  } else if (str == "ineq") {
    return Int::CompareOp::kNeq;
  } else if (str == "ilss") {
    return Int::CompareOp::kLss;
  } else if (str == "ileq") {
    return Int::CompareOp::kLeq;
  } else if (str == "igeq") {
    return Int::CompareOp::kGeq;
  } else if (str == "igtr") {
    return Int::CompareOp::kGtr;
  } else {
    return std::nullopt;
  }
}

std::string ToString(Int::CompareOp op) {
  switch (op) {
    case Int::CompareOp::kEq:
      return "ieq";
    case Int::CompareOp::kNeq:
      return "ineq";
    case Int::CompareOp::kLss:
      return "ilss";
    case Int::CompareOp::kLeq:
      return "ileq";
    case Int::CompareOp::kGeq:
      return "igeq";
    case Int::CompareOp::kGtr:
      return "igtr";
  }
}

std::optional<Int::BinaryOp> ToIntBinaryOp(std::string_view str) {
  if (str == "iadd") {
    return Int::BinaryOp::kAdd;
  } else if (str == "isub") {
    return Int::BinaryOp::kSub;
  } else if (str == "imul") {
    return Int::BinaryOp::kMul;
  } else if (str == "idiv") {
    return Int::BinaryOp::kDiv;
  } else if (str == "irem") {
    return Int::BinaryOp::kRem;
  } else if (str == "iand") {
    return Int::BinaryOp::kAnd;
  } else if (str == "ior") {
    return Int::BinaryOp::kOr;
  } else if (str == "ixor") {
    return Int::BinaryOp::kXor;
  } else if (str == "iandnot") {
    return Int::BinaryOp::kAndNot;
  } else {
    return std::nullopt;
  }
}

std::string ToString(Int::BinaryOp op) {
  switch (op) {
    case Int::BinaryOp::kAdd:
      return "iadd";
    case Int::BinaryOp::kSub:
      return "isub";
    case Int::BinaryOp::kMul:
      return "imul";
    case Int::BinaryOp::kDiv:
      return "idiv";
    case Int::BinaryOp::kRem:
      return "irem";
    case Int::BinaryOp::kAnd:
      return "iand";
    case Int::BinaryOp::kOr:
      return "ior";
    case Int::BinaryOp::kXor:
      return "ixor";
    case Int::BinaryOp::kAndNot:
      return "iandnot";
  }
}

std::optional<Int::ShiftOp> ToIntShiftOp(std::string_view str) {
  if (str == "ishl") {
    return Int::ShiftOp::kLeft;
  } else if (str == "ishr") {
    return Int::ShiftOp::kRight;
  } else {
    return std::nullopt;
  }
}

std::string ToString(Int::ShiftOp op) {
  switch (op) {
    case Int::ShiftOp::kLeft:
      return "ishl";
    case Int::ShiftOp::kRight:
      return "ishr";
  }
}

}  // namespace common

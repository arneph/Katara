//
//  constants.cc
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "constants.h"

#include "src/common/logging/logging.h"

namespace lang {
namespace constants {

using ::common::atomics::Bool;
using ::common::atomics::Int;
using ::common::logging::fail;

std::string Value::ToString() const {
  switch (kind()) {
    case Kind::kBool:
      return Bool::ToString(AsBool());
    case Kind::kInt:
      return AsInt().ToString();
    case Kind::kString:
      return AsString();
  }
}

bool Compare(Value x, tokens::Token tok, Value y) {
  if (x.kind() != y.kind()) {
    fail("incompatible operand types");
  }
  switch (x.kind()) {
    case Value::Kind::kBool: {
      Bool::BinaryOp op = [tok]() {
        switch (tok) {
          case tokens::kEql:
            return Bool::BinaryOp::kEq;
          case tokens::kNeq:
            return Bool::BinaryOp::kNeq;
          default:
            fail("unexpected compare op");
        }
      }();
      return Bool::Compute(x.AsBool(), op, y.AsBool());
    }
    case Value::Kind::kInt: {
      if (Int::CanCompare(x.AsInt(), y.AsInt())) {
        return false;
      }
      Int::CompareOp op = [tok]() {
        switch (tok) {
          case tokens::kEql:
            return Int::CompareOp::kEq;
          case tokens::kNeq:
            return Int::CompareOp::kNeq;
          case tokens::kLss:
            return Int::CompareOp::kLss;
          case tokens::kLeq:
            return Int::CompareOp::kLeq;
          case tokens::kGeq:
            return Int::CompareOp::kGeq;
          case tokens::kGtr:
            return Int::CompareOp::kGtr;
          default:
            fail("unexpected compare op");
        }
      }();
      return Int::Compare(x.AsInt(), op, y.AsInt());
    }
    case Value::Kind::kString:
      switch (tok) {
        case tokens::kEql:
          return x.AsString() == y.AsString();
        case tokens::kNeq:
          return x.AsString() != y.AsString();
        case tokens::kLss:
          return x.AsString() < y.AsString();
        case tokens::kLeq:
          return x.AsString() <= y.AsString();
        case tokens::kGeq:
          return x.AsString() >= y.AsString();
        case tokens::kGtr:
          return x.AsString() > y.AsString();
        default:
          fail("unexpected compare op");
      }
  }
}

Value BinaryOp(Value x, tokens::Token tok, Value y) {
  if (x.kind() != y.kind()) {
    fail("incompatible operand types");
  }
  switch (x.kind()) {
    case Value::Kind::kBool: {
      Bool::BinaryOp op = [tok]() {
        switch (tok) {
          case tokens::kLAnd:
            return Bool::BinaryOp::kAnd;
          case tokens::kLOr:
            return Bool::BinaryOp::kOr;
          default:
            fail("unexpected binary op");
        }
      }();
      return constants::Value(Bool::Compute(x.AsBool(), op, y.AsBool()));
    }
    case Value::Kind::kInt: {
      if (!Int::CanCompute(x.AsInt(), y.AsInt())) {
        fail("disallowed operation");
      }
      Int::BinaryOp op = [tok]() {
        switch (tok) {
          case tokens::kAdd:
            return Int::BinaryOp::kAdd;
          case tokens::kSub:
            return Int::BinaryOp::kSub;
          case tokens::kMul:
            return Int::BinaryOp::kMul;
          case tokens::kQuo:
            return Int::BinaryOp::kDiv;
          case tokens::kRem:
            return Int::BinaryOp::kRem;
          case tokens::kAnd:
            return Int::BinaryOp::kAnd;
          case tokens::kOr:
            return Int::BinaryOp::kOr;
          case tokens::kXor:
            return Int::BinaryOp::kXor;
          case tokens::kAndNot:
            return Int::BinaryOp::kAndNot;
          default:
            fail("unexpected binary op");
        }
      }();
      return constants::Value(Int::Compute(x.AsInt(), op, y.AsInt()));
    }
    case Value::Kind::kString:
      switch (tok) {
        case tokens::kAdd:
          return constants::Value(x.AsString() + y.AsString());
        default:
          fail("unexpected compare op");
      }
  }
}

Value ShiftOp(Value x, tokens::Token tok, Value y) {
  if (x.kind() != Value::Kind::kInt || y.kind() != Value::Kind::kInt ||
      !common::atomics::IsUnsigned(y.AsInt().type())) {
    fail("unexpected shift operand type");
  }
  Int::ShiftOp op = [tok]() {
    switch (tok) {
      case tokens::kShl:
        return Int::ShiftOp::kLeft;
      case tokens::kShr:
        return Int::ShiftOp::kRight;
      default:
        fail("unexpected shift op");
    }
  }();
  return constants::Value(Int::Shift(x.AsInt(), op, y.AsInt()));
}

Value UnaryOp(tokens::Token tok, Value x) {
  switch (x.kind()) {
    case Value::Kind::kBool:
      switch (tok) {
        case tokens::kNot:
          return constants::Value(!x.AsBool());
        default:
          fail("unexpected unary op");
      }
    case Value::Kind::kInt: {
      if (tok == tokens::kAdd) {
        return x;
      }
      Int::UnaryOp op = [tok]() {
        switch (tok) {
          case tokens::kSub:
            return Int::UnaryOp::kNeg;
          case tokens::kXor:
            return Int::UnaryOp::kNot;
          default:
            fail("unexpected unary op");
        }
      }();
      if (!Int::CanCompute(op, x.AsInt())) {
        fail("disallowed operation");
      }
      return constants::Value(Int::Compute(op, x.AsInt()));
    }
    case Value::Kind::kString:
      fail("unexpected unary operand type");
  }
}

}  // namespace constants
}  // namespace lang

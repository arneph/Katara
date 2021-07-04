//
//  constants.cc
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "constants.h"

namespace lang {
namespace constants {

std::string Value::ToString() const {
  switch (kind()) {
    case Kind::kBool:
      return common::Bool::ToString(AsBool());
    case Kind::kInt:
      return AsInt().ToString();
    case Kind::kString:
      return AsString();
  }
}

bool Compare(Value x, tokens::Token tok, Value y) {
  if (x.kind() != y.kind()) {
    throw "incompatible operand types";
  }
  switch (x.kind()) {
    case Value::Kind::kBool: {
      common::Bool::BinaryOp op = [tok]() {
        switch (tok) {
          case tokens::kEql:
            return common::Bool::BinaryOp::kEq;
          case tokens::kNeq:
            return common::Bool::BinaryOp::kNeq;
          default:
            throw "unexpected compare op";
        }
      }();
      return common::Bool::Compute(x.AsBool(), op, y.AsBool());
    }
    case Value::Kind::kInt: {
      if (common::Int::CanCompare(x.AsInt(), y.AsInt())) {
        return false;
      }
      common::Int::CompareOp op = [tok]() {
        switch (tok) {
          case tokens::kEql:
            return common::Int::CompareOp::kEq;
          case tokens::kNeq:
            return common::Int::CompareOp::kNeq;
          case tokens::kLss:
            return common::Int::CompareOp::kLss;
          case tokens::kLeq:
            return common::Int::CompareOp::kLeq;
          case tokens::kGeq:
            return common::Int::CompareOp::kGeq;
          case tokens::kGtr:
            return common::Int::CompareOp::kGtr;
          default:
            throw "unexpected compare op";
        }
      }();
      return common::Int::Compare(x.AsInt(), op, y.AsInt());
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
          throw "unexpected compare op";
      }
  }
}

Value BinaryOp(Value x, tokens::Token tok, Value y) {
  if (x.kind() != y.kind()) {
    throw "incompatible operand types";
  }
  switch (x.kind()) {
    case Value::Kind::kBool: {
      common::Bool::BinaryOp op = [tok]() {
        switch (tok) {
          case tokens::kLAnd:
            return common::Bool::BinaryOp::kAnd;
          case tokens::kLOr:
            return common::Bool::BinaryOp::kOr;
          default:
            throw "unexpected binary op";
        }
      }();
      return constants::Value(common::Bool::Compute(x.AsBool(), op, y.AsBool()));
    }
    case Value::Kind::kInt: {
      if (!common::Int::CanCompute(x.AsInt(), y.AsInt())) {
        throw "disallowed operation";
      }
      common::Int::BinaryOp op = [tok]() {
        switch (tok) {
          case tokens::kAdd:
            return common::Int::BinaryOp::kAdd;
          case tokens::kSub:
            return common::Int::BinaryOp::kSub;
          case tokens::kMul:
            return common::Int::BinaryOp::kMul;
          case tokens::kQuo:
            return common::Int::BinaryOp::kDiv;
          case tokens::kRem:
            return common::Int::BinaryOp::kRem;
          case tokens::kAnd:
            return common::Int::BinaryOp::kAnd;
          case tokens::kOr:
            return common::Int::BinaryOp::kOr;
          case tokens::kXor:
            return common::Int::BinaryOp::kXor;
          case tokens::kAndNot:
            return common::Int::BinaryOp::kAndNot;
          default:
            throw "unexpected binary op";
        }
      }();
      return constants::Value(common::Int::Compute(x.AsInt(), op, y.AsInt()));
    }
    case Value::Kind::kString:
      switch (tok) {
        case tokens::kAdd:
          return constants::Value(x.AsString() + y.AsString());
        default:
          throw "unexpected compare op";
      }
  }
}

Value ShiftOp(Value x, tokens::Token tok, Value y) {
  if (x.kind() != Value::Kind::kInt || y.kind() != Value::Kind::kInt ||
      !common::IsUnsigned(y.AsInt().type())) {
    throw "unexpected shift operand type";
  }
  common::Int::ShiftOp op = [tok]() {
    switch (tok) {
      case tokens::kShl:
        return common::Int::ShiftOp::kLeft;
      case tokens::kShr:
        return common::Int::ShiftOp::kRight;
      default:
        throw "unexpected shift op";
    }
  }();
  return constants::Value(common::Int::Shift(x.AsInt(), op, y.AsInt()));
}

Value UnaryOp(tokens::Token tok, Value x) {
  switch (x.kind()) {
    case Value::Kind::kBool:
      switch (tok) {
        case tokens::kNot:
          return constants::Value(!x.AsBool());
        default:
          throw "unexpected unary op";
      }
    case Value::Kind::kInt: {
      if (tok == tokens::kAdd) {
        return x;
      }
      common::Int::UnaryOp op = [tok]() {
        switch (tok) {
          case tokens::kSub:
            return common::Int::UnaryOp::kNeg;
          case tokens::kXor:
            return common::Int::UnaryOp::kNot;
          default:
            throw "unexpected unary op";
        }
      }();
      if (!common::Int::CanCompute(op, x.AsInt())) {
        throw "disallowed operation";
      }
      return constants::Value(common::Int::Compute(op, x.AsInt()));
    }
    case Value::Kind::kString:
      throw "unexpected unary operand type";
  }
}

}  // namespace constants
}  // namespace lang

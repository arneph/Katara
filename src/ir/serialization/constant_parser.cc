//
//  constant_parser.cc
//  Katara
//
//  Created by Arne Philipeit on 6/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "constant_parser.h"

#include "src/common/atomics/atomics.h"
#include "src/common/logging/logging.h"

namespace ir_serialization {

std::shared_ptr<ir::Constant> ConstantParser::ParseConstant(const ir::Type* expected_type) {
  switch (scanner().token()) {
    case Scanner::kAddress:
      if (expected_type != nullptr && expected_type != ir::pointer_type()) {
        common::fail(scanner().PositionString() + ": unexpected '0x'");
      }
      return ParsePointerConstant();
    case Scanner::kAtSign:
      if (expected_type != nullptr && expected_type != ir::func_type()) {
        common::fail(scanner().PositionString() + ": unexpected '@'");
      }
      return ParseFuncConstant();
    case Scanner::kHashSign:
      return ParseBoolOrIntConstant(expected_type);
    default:
      scanner().FailForUnexpectedToken({Scanner::kAtSign, Scanner::kHashSign, Scanner::kAddress});
      return nullptr;
  }
}

// PointerConstant ::= '0x' Number
std::shared_ptr<ir::PointerConstant> ConstantParser::ParsePointerConstant() {
  uint64_t number = scanner().token_address().AsUint64();
  scanner().Next();

  return ir::ToPointerConstant(number);
}

// FuncConstant ::= '@' Number
std::shared_ptr<ir::FuncConstant> ConstantParser::ParseFuncConstant() {
  scanner().ConsumeToken(Scanner::kAtSign);
  ir::func_num_t number = scanner().ConsumeInt64();
  if (number != ir::kNoFuncNum) {
    number += func_num_offset_;
  }

  return ir::ToFuncConstant(number);
}

// Constant ::= '#t' | '#f' | '#' Number (':' Type)?
std::shared_ptr<ir::Constant> ConstantParser::ParseBoolOrIntConstant(
    const ir::Type* expected_type) {
  scanner().ConsumeToken(Scanner::kHashSign);

  if (scanner().token() == Scanner::kIdentifier) {
    std::string str = scanner().ConsumeIdentifier();
    if (str == "f" || str == "t") {
      if (expected_type != nullptr && expected_type != ir::bool_type()) {
        common::fail(scanner().PositionString() + ": unexpected 'f' or 't'");
      }

      return (str == "f") ? ir::False() : ir::True();
    } else {
      common::fail(scanner().PositionString() + ": expected number, 't' or 'f'");
    }
  }

  if (scanner().token() != Scanner::kNumber) {
    common::fail(scanner().PositionString() + ": expected number, 't' or 'f'");
  }
  common::Int value = scanner().token_number();
  scanner().Next();

  common::IntType int_type;
  if (scanner().token() == Scanner::kColon) {
    scanner().ConsumeToken(Scanner::kColon);
    auto type = type_parser()->ParseType();
    if (type->type_kind() != ir::TypeKind::kInt) {
      common::fail(scanner().PositionString() + ": expected int type");
    } else if (expected_type != nullptr && expected_type != type) {
      common::fail(scanner().PositionString() + ": expected '" + expected_type->RefString() +
                   "'; got '" + type->RefString() + "'");
    }
    int_type = static_cast<const ir::IntType*>(type)->int_type();

  } else {
    if (expected_type == nullptr) {
      scanner().FailForUnexpectedToken({Scanner::kColon});
    } else if (expected_type->type_kind() != ir::TypeKind::kInt) {
      common::fail(scanner().PositionString() + ": expected '" + expected_type->RefString() + "'");
    }

    int_type = static_cast<const ir::IntType*>(expected_type)->int_type();
  }
  value = value.ConvertTo(int_type);
  return ir::ToIntConstant(value);
}

}  // namespace ir_serialization

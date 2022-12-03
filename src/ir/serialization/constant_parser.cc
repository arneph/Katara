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
        issue_tracker().Add(ir_issues::IssueKind::kUnexpectedAddress, scanner().token_start(),
                            "unexpected address");
        scanner().Next();
        return nullptr;
      }
      return ParsePointerConstant();
    case Scanner::kAtSign:
      if (expected_type != nullptr && expected_type != ir::func_type()) {
        issue_tracker().Add(ir_issues::IssueKind::kUnexpectedFuncConstant, scanner().token_start(),
                            "unexpected function constant");
        scanner().Next();
        return nullptr;
      }
      return ParseFuncConstant();
    case Scanner::kHashSign:
      return ParseBoolOrIntConstant(expected_type);
    default:
      scanner().AddErrorForUnexpectedToken(
          {Scanner::kAtSign, Scanner::kHashSign, Scanner::kAddress});
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
  ir::func_num_t number = scanner().ConsumeInt64().value_or(ir::kNoFuncNum);
  if (number != ir::kNoFuncNum) {
    number += func_num_offset_;
  }

  return ir::ToFuncConstant(number);
}

// Constant ::= '#t' | '#f' | '#' Number (':' Type)?
std::shared_ptr<ir::Constant> ConstantParser::ParseBoolOrIntConstant(
    const ir::Type* expected_type) {
  common::pos_t hash_sign_pos = scanner().token_start();
  scanner().ConsumeToken(Scanner::kHashSign);

  if (scanner().token() == Scanner::kIdentifier) {
    std::string str = scanner().ConsumeIdentifier().value();
    if (str == "f" || str == "t") {
      if (expected_type != nullptr && expected_type != ir::bool_type()) {
        issue_tracker().Add(ir_issues::IssueKind::kUnexpectedType, hash_sign_pos,
                            "expected '" + expected_type->RefString() + "'; got 'b'");
        return nullptr;
      }

      return (str == "f") ? ir::False() : ir::True();
    } else {
      issue_tracker().Add(ir_issues::IssueKind::kUnexpectedBoolConstant, hash_sign_pos,
                          "unexpected bool constant");
      return nullptr;
    }
  }

  if (scanner().token() != Scanner::kNumber) {
    scanner().AddErrorForUnexpectedToken({Scanner::kNumber, Scanner::kIdentifier});
    return nullptr;
  }
  common::Int value = scanner().token_number();
  scanner().Next();

  common::IntType int_type = common::IntType::kI64;
  if (scanner().token() == Scanner::kColon) {
    scanner().ConsumeToken(Scanner::kColon);
    common::pos_t type_pos = scanner().token_start();
    auto type = type_parser()->ParseType();
    if (type->type_kind() != ir::TypeKind::kInt) {
      issue_tracker().Add(ir_issues::IssueKind::kUnexpectedType, type_pos,
                          "expected int type; got '" + type->RefString() + "'");
      return nullptr;
    } else if (expected_type != nullptr && expected_type != type) {
      issue_tracker().Add(
          ir_issues::IssueKind::kUnexpectedType, type_pos,
          "expected '" + expected_type->RefString() + "'; got '" + type->RefString() + "'");
      return nullptr;
    }
    int_type = static_cast<const ir::IntType*>(type)->int_type();

  } else {
    if (expected_type == nullptr) {
      scanner().AddErrorForUnexpectedToken({Scanner::kColon});
    } else if (expected_type->type_kind() != ir::TypeKind::kInt) {
      issue_tracker().Add(ir_issues::IssueKind::kUnexpectedType, hash_sign_pos,
                          "expected '" + expected_type->RefString() + "'; got int type");
    } else {
      int_type = static_cast<const ir::IntType*>(expected_type)->int_type();
    }
  }
  value = value.ConvertTo(int_type);
  return ir::ToIntConstant(value);
}

}  // namespace ir_serialization

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
#include "src/common/positions/positions.h"

namespace ir_serialization {

using ::common::atomics::Int;
using ::common::atomics::IntType;
using ::common::positions::kNoRange;
using ::common::positions::pos_t;
using ::common::positions::range_t;

ConstantParser::ConstantParseResult ConstantParser::NoConstantParseResult() {
  return ConstantParseResult{
      .constant = nullptr,
      .range = kNoRange,
  };
}

ConstantParser::ConstantParseResult ConstantParser::ParseConstant(const ir::Type* expected_type) {
  switch (scanner().token()) {
    case Scanner::kAddress:
      if (expected_type != nullptr && expected_type != ir::pointer_type()) {
        issue_tracker().Add(ir_issues::IssueKind::kUnexpectedAddress, scanner().token_start(),
                            "unexpected address");
        scanner().Next();
        return NoConstantParseResult();
      }
      return ParsePointerConstant();
    case Scanner::kAtSign:
      if (expected_type != nullptr && expected_type != ir::func_type()) {
        issue_tracker().Add(ir_issues::IssueKind::kUnexpectedFuncConstant, scanner().token_start(),
                            "unexpected function constant");
        scanner().Next();
        return NoConstantParseResult();
      }
      return ParseFuncConstant();
    case Scanner::kHashSign:
      return ParseBoolOrIntConstant(expected_type);
    default:
      scanner().AddErrorForUnexpectedToken(
          {Scanner::kAtSign, Scanner::kHashSign, Scanner::kAddress});
      return NoConstantParseResult();
  }
}

// PointerConstant ::= '0x' Number
ConstantParser::ConstantParseResult ConstantParser::ParsePointerConstant() {
  range_t number_range = scanner().token_range();
  uint64_t number = scanner().token_address().AsUint64();
  scanner().Next();

  return ConstantParseResult{
      .constant = ir::ToPointerConstant(number),
      .range = number_range,
  };
}

// FuncConstant ::= '@' Number
ConstantParser::ConstantParseResult ConstantParser::ParseFuncConstant() {
  pos_t start = scanner().token_start();
  scanner().ConsumeToken(Scanner::kAtSign);
  pos_t end = scanner().token_end();
  ir::func_num_t number = scanner().ConsumeInt64().value_or(ir::kNoFuncNum);
  if (number != ir::kNoFuncNum) {
    number += func_num_offset_;
  }

  return ConstantParseResult{
      .constant = ir::ToFuncConstant(number),
      .range =
          range_t{
              .start = start,
              .end = end,
          },
  };
}

// Constant ::= '#t' | '#f' | '#' Number (':' Type)?
ConstantParser::ConstantParseResult ConstantParser::ParseBoolOrIntConstant(
    const ir::Type* expected_type) {
  pos_t start = scanner().token_start();
  scanner().ConsumeToken(Scanner::kHashSign);

  if (scanner().token() == Scanner::kIdentifier) {
    pos_t end = scanner().token_end();
    std::string str = scanner().ConsumeIdentifier().value();
    range_t range = range_t{
        .start = start,
        .end = end,
    };
    if (str == "f" || str == "t") {
      if (expected_type != nullptr && expected_type != ir::bool_type()) {
        issue_tracker().Add(ir_issues::IssueKind::kUnexpectedType, range,
                            "expected '" + expected_type->RefString() + "'; got 'b'");
        return NoConstantParseResult();
      }

      return ConstantParseResult{
          .constant = (str == "f") ? ir::False() : ir::True(),
          .range = range,
      };
    } else {
      issue_tracker().Add(ir_issues::IssueKind::kUnexpectedBoolConstant, range,
                          "unexpected bool constant");
      return NoConstantParseResult();
    }
  }

  if (scanner().token() != Scanner::kNumber) {
    scanner().AddErrorForUnexpectedToken({Scanner::kNumber, Scanner::kIdentifier});
    return NoConstantParseResult();
  }
  pos_t end = scanner().token_end();
  Int value = scanner().token_number();
  scanner().Next();

  IntType int_type = IntType::kI64;
  if (scanner().token() == Scanner::kColon) {
    scanner().ConsumeToken(Scanner::kColon);
    const auto& [type, type_range] = type_parser()->ParseType();
    end = type_range.end;
    if (type->type_kind() != ir::TypeKind::kInt) {
      issue_tracker().Add(ir_issues::IssueKind::kUnexpectedType, type_range,
                          "expected int type; got '" + type->RefString() + "'");
      return NoConstantParseResult();
    } else if (expected_type != nullptr && expected_type != type) {
      issue_tracker().Add(
          ir_issues::IssueKind::kUnexpectedType, type_range,
          "expected '" + expected_type->RefString() + "'; got '" + type->RefString() + "'");
      return NoConstantParseResult();
    }
    int_type = static_cast<const ir::IntType*>(type)->int_type();

  } else {
    if (expected_type == nullptr) {
      scanner().AddErrorForUnexpectedToken({Scanner::kColon});
    } else if (expected_type->type_kind() != ir::TypeKind::kInt) {
      range_t range = range_t{
          .start = start,
          .end = end,
      };
      issue_tracker().Add(ir_issues::IssueKind::kUnexpectedType, range,
                          "expected '" + expected_type->RefString() + "'; got int type");
    } else {
      int_type = static_cast<const ir::IntType*>(expected_type)->int_type();
    }
  }
  value = value.ConvertTo(int_type);
  return ConstantParseResult{
      .constant = ir::ToIntConstant(value),
      .range =
          range_t{
              .start = start,
              .end = end,
          },
  };
}

}  // namespace ir_serialization

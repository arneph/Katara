//
//  func_parser.cc
//  Katara
//
//  Created by Arne Philipeit on 6/4/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "func_parser.h"

#include "src/common/logging/logging.h"

namespace lang {
namespace ir_serialization {

std::unique_ptr<ir::Instr> FuncParser::ParseInstrWithResults(
    std::vector<std::shared_ptr<ir::Computed>> results, std::string instr_name) {
  if (instr_name == "panic") {
    if (results.size() != 0) {
      common::fail(scanner().PositionString() + ": expected no results for panic instruction");
    }
    return ParsePanicInstr();
  } else if (instr_name == "make_shared") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() +
                   ": expected one results for make_shared instruction");
    }
    return ParseMakeSharedInstr(results.front());
  } else if (instr_name == "copy_shared") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() +
                   ": expected one results for copy_shared instruction");
    }
    return ParseCopySharedInstr(results.front());
  } else if (instr_name == "delete_shared") {
    if (results.size() != 0) {
      common::fail(scanner().PositionString() +
                   ": expected no results for delete_shared instruction");
    }
    return ParseDeleteSharedInstr();
  } else if (instr_name == "make_unique") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() +
                   ": expected one results for make_unique instruction");
    }
    return ParseMakeUniqueInstr(results.front());
  } else if (instr_name == "delete_unique") {
    if (results.size() != 0) {
      common::fail(scanner().PositionString() +
                   ": expected no results for delete_unique instruction");
    }
    return ParseDeleteUniqueInstr();
  } else if (instr_name == "str_index") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() + ": expected one results for str_index instruction");
    }
    return ParseStringIndexInstr(results.front());
  } else if (instr_name == "str_cat") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() + ": expected one results for str_cat instruction");
    }
    return ParseStringConcatInstr(results.front());
  } else {
    return ::ir_serialization::FuncParser::ParseInstrWithResults(results, instr_name);
  }
}

// PanicInstr ::= 'panic' Value NL
std::unique_ptr<ir_ext::PanicInstr> FuncParser::ParsePanicInstr() {
  std::shared_ptr<ir::Value> reason = ParseValue(ir_ext::string());
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::PanicInstr>(reason);
}

// MakeSharedPointerInstr ::= 'panic' Value NL
std::unique_ptr<ir_ext::MakeSharedPointerInstr> FuncParser::ParseMakeSharedInstr(
    std::shared_ptr<ir::Computed> result) {
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::MakeSharedPointerInstr>(result);
}

std::unique_ptr<ir_ext::CopySharedPointerInstr> FuncParser::ParseCopySharedInstr(
    std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Computed> copied_shared_pointer = ParseComputedValue(result->type());
  scanner().ConsumeToken(::ir_serialization::Scanner::kComma);

  std::shared_ptr<ir::Value> pointer_offset = ParseValue(ir::i64());
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::CopySharedPointerInstr>(result, copied_shared_pointer,
                                                          pointer_offset);
}

std::unique_ptr<ir_ext::DeleteSharedPointerInstr> FuncParser::ParseDeleteSharedInstr() {
  std::shared_ptr<ir::Computed> deleted_shared_pointer = ParseComputedValue(nullptr);
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::DeleteSharedPointerInstr>(deleted_shared_pointer);
}

std::unique_ptr<ir_ext::MakeUniquePointerInstr> FuncParser::ParseMakeUniqueInstr(
    std::shared_ptr<ir::Computed> result) {
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::MakeUniquePointerInstr>(result);
}

std::unique_ptr<ir_ext::DeleteUniquePointerInstr> FuncParser::ParseDeleteUniqueInstr() {
  std::shared_ptr<ir::Computed> deleted_unique_pointer = ParseComputedValue(nullptr);
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::DeleteUniquePointerInstr>(deleted_unique_pointer);
}

std::unique_ptr<ir_ext::StringIndexInstr> FuncParser::ParseStringIndexInstr(
    std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> string_operand = ParseValue(ir_ext::string());
  scanner().ConsumeToken(::ir_serialization::Scanner::kComma);

  std::shared_ptr<ir::Value> index_operand = ParseValue(ir::i64());
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::StringIndexInstr>(result, string_operand, index_operand);
}

std::unique_ptr<ir_ext::StringConcatInstr> FuncParser::ParseStringConcatInstr(
    std::shared_ptr<ir::Computed> result) {
  std::vector<std::shared_ptr<ir::Value>> operands{ParseValue(ir_ext::string())};
  while (scanner().token() != ::ir_serialization::Scanner::kNewLine) {
    scanner().ConsumeToken(::ir_serialization::Scanner::kComma);

    operands.push_back(ParseValue(ir_ext::string()));
  }
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::StringConcatInstr>(result, operands);
}

}  // namespace ir_serialization
}  // namespace lang

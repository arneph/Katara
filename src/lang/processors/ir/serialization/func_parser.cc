//
//  func_parser.cc
//  Katara
//
//  Created by Arne Philipeit on 6/4/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "func_parser.h"

namespace lang {
namespace ir_serialization {

std::unique_ptr<ir::Instr> FuncParser::ParseInstrWithResults(
    std::vector<std::shared_ptr<ir::Computed>> results, std::string instr_name) {
  if (instr_name == "panic") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected no results for panic instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return nullptr;
    }
    return ParsePanicInstr();
  } else if (instr_name == "make_shared") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected one result for make_shared instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return nullptr;
    }
    return ParseMakeSharedInstr(results.front());
  } else if (instr_name == "copy_shared") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected one result for copy_shared instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return nullptr;
    }
    return ParseCopySharedInstr(results.front());
  } else if (instr_name == "delete_shared") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected no results for delete_shared instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return nullptr;
    }
    return ParseDeleteSharedInstr();
  } else if (instr_name == "make_unique") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected one result for make_unique instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return nullptr;
    }
    return ParseMakeUniqueInstr(results.front());
  } else if (instr_name == "delete_unique") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected no results for delete_unique instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return nullptr;
    }
    return ParseDeleteUniqueInstr();
  } else if (instr_name == "str_index") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected one result for str_index instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return nullptr;
    }
    return ParseStringIndexInstr(results.front());
  } else if (instr_name == "str_cat") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected one result for str_cat instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return nullptr;
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

std::unique_ptr<ir_ext::MakeSharedPointerInstr> FuncParser::ParseMakeSharedInstr(
    std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> size = ParseValue(ir::i64());
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::MakeSharedPointerInstr>(result, size);
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
  std::shared_ptr<ir::Value> size = ParseValue(ir::i64());
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::MakeUniquePointerInstr>(result, size);
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

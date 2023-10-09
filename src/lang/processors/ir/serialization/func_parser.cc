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

using ::common::positions::range_t;

::ir_serialization::FuncParser::InstrParseResult FuncParser::ParseInstrWithResults(
    std::vector<std::shared_ptr<ir::Computed>> results, std::string instr_name) {
  if (instr_name == "panic") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected no results for panic instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParsePanicInstr();
  } else if (instr_name == "make_shared") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected one result for make_shared instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseMakeSharedInstr(results.front());
  } else if (instr_name == "copy_shared") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected one result for copy_shared instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseCopySharedInstr(results.front());
  } else if (instr_name == "delete_shared") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected no results for delete_shared instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseDeleteSharedInstr();
  } else if (instr_name == "make_unique") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected one result for make_unique instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseMakeUniqueInstr(results.front());
  } else if (instr_name == "delete_unique") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected no results for delete_unique instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseDeleteUniqueInstr();
  } else if (instr_name == "str_index") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected one result for str_index instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseStringIndexInstr(results.front());
  } else if (instr_name == "str_cat") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPanicInstrHasResults, scanner().token_start(),
                          "expected one result for str_cat instruction");
      scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseStringConcatInstr(results.front());
  } else {
    return ::ir_serialization::FuncParser::ParseInstrWithResults(results, instr_name);
  }
}

// PanicInstr ::= 'panic' Value NL
::ir_serialization::FuncParser::InstrParseResult FuncParser::ParsePanicInstr() {
  const auto& [reason, reason_range] = ParseValue(ir_ext::string());
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir_ext::PanicInstr>(reason),
      .arg_ranges = {reason_range},
      .args_range = reason_range,
  };
}

::ir_serialization::FuncParser::InstrParseResult FuncParser::ParseMakeSharedInstr(
    std::shared_ptr<ir::Computed> result) {
  const auto& [size, size_range] = ParseValue(ir::i64());
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir_ext::MakeSharedPointerInstr>(result, size),
      .arg_ranges = {size_range},
      .args_range = size_range,
  };
}

FuncParser::InstrParseResult FuncParser::ParseCopySharedInstr(
    std::shared_ptr<ir::Computed> result) {
  const auto& [copied_shared_pointer, copied_shared_pointer_range] =
      ParseComputedValue(result->type());
  scanner().ConsumeToken(::ir_serialization::Scanner::kComma);

  const auto& [pointer_offset, pointer_offset_range] = ParseValue(ir::i64());
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir_ext::CopySharedPointerInstr>(result, copied_shared_pointer,
                                                                pointer_offset),
      .arg_ranges = {copied_shared_pointer_range, pointer_offset_range},
      .args_range =
          range_t{
              .start = copied_shared_pointer_range.start,
              .end = pointer_offset_range.end,
          },
  };
}

::ir_serialization::FuncParser::InstrParseResult FuncParser::ParseDeleteSharedInstr() {
  const auto& [deleted_shared_pointer, deleted_shared_pointer_range] = ParseComputedValue(nullptr);
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir_ext::DeleteSharedPointerInstr>(deleted_shared_pointer),
      .arg_ranges = {deleted_shared_pointer_range},
      .args_range = deleted_shared_pointer_range,
  };
}

::ir_serialization::FuncParser::InstrParseResult FuncParser::ParseMakeUniqueInstr(
    std::shared_ptr<ir::Computed> result) {
  const auto& [size, size_range] = ParseValue(ir::i64());
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir_ext::MakeUniquePointerInstr>(result, size),
      .arg_ranges = {size_range},
      .args_range = size_range,
  };
}

::ir_serialization::FuncParser::InstrParseResult FuncParser::ParseDeleteUniqueInstr() {
  const auto& [deleted_unique_pointer, deleted_unique_pointer_range] = ParseComputedValue(nullptr);
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir_ext::DeleteUniquePointerInstr>(deleted_unique_pointer),
      .arg_ranges = {deleted_unique_pointer_range},
      .args_range = deleted_unique_pointer_range,
  };
}

::ir_serialization::FuncParser::InstrParseResult FuncParser::ParseStringIndexInstr(
    std::shared_ptr<ir::Computed> result) {
  const auto& [string_operand, string_operand_range] = ParseValue(ir_ext::string());
  scanner().ConsumeToken(::ir_serialization::Scanner::kComma);

  const auto& [index_operand, index_operand_range] = ParseValue(ir::i64());
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir_ext::StringIndexInstr>(result, string_operand, index_operand),
      .arg_ranges = {string_operand_range, index_operand_range},
      .args_range =
          range_t{
              .start = string_operand_range.start,
              .end = index_operand_range.end,
          },
  };
}

::ir_serialization::FuncParser::InstrParseResult FuncParser::ParseStringConcatInstr(
    std::shared_ptr<ir::Computed> result) {
  ValuesParseResult parsed_operands = ParseValues(ir_ext::string());
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir_ext::StringConcatInstr>(result, parsed_operands.values),
      .arg_ranges = parsed_operands.value_ranges,
      .args_range = parsed_operands.range,
  };
}

}  // namespace ir_serialization
}  // namespace lang

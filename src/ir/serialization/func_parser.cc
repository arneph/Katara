//
//  parser.cc
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "func_parser.h"

#include "src/common/positions/positions.h"
#include "src/ir/serialization/positions_util.h"

namespace ir_serialization {

using ::common::atomics::Bool;
using ::common::atomics::Int;
using ::common::positions::kNoRange;
using ::common::positions::pos_t;
using ::common::positions::range_t;

// Func ::= FuncNum Identifier? FuncArgs '=>' FuncResultTypes FuncBody
ir::Func* FuncParser::ParseFunc() {
  if (auto [func_num, func_num_range] = ParseFuncNumber(); func_num != ir::kNoFuncNum) {
    func_ = program()->AddFunc(func_num);
    func_positions_ = FuncPositions();
    func_positions_.set_number(func_num_range);
  } else {
    return nullptr;
  }

  if (scanner().token() == Scanner::kIdentifier) {
    func_positions_.set_name(scanner().token_range());
    func_->set_name(scanner().ConsumeIdentifier().value_or(""));
    if (func_->name() == "main") {
      program()->set_entry_func_num(func_->number());
    }
  }

  ParseFuncArgs();
  scanner().ConsumeToken(Scanner::kArrow);
  ParseFuncResultTypes();
  ParseFuncBody();

  program_positions_.AddFuncPositions(func_, func_positions_);
  return func_;
}

// FuncNum ::= '@' Number
FuncParser::FuncNumberParseResult FuncParser::ParseFuncNumber() {
  pos_t func_num_start = scanner().token_start();
  if (!scanner().ConsumeToken(Scanner::kAtSign)) {
    scanner().SkipPastTokenSequence({Scanner::kNewLine, Scanner::kCurlyBracketClose});
    return FuncNumberParseResult{
        .func_num = ir::kNoFuncNum,
        .range = kNoRange,
    };
  }

  pos_t func_num_end = scanner().token_end();
  ir::func_num_t func_num = scanner().ConsumeInt64().value_or(ir::kNoFuncNum);
  if (func_num == ir::kNoFuncNum) {
    return FuncNumberParseResult{
        .func_num = ir::kNoFuncNum,
        .range = kNoRange,
    };
  }
  func_num += func_num_offset_;

  range_t func_num_range = range_t{
      .start = func_num_start,
      .end = func_num_end,
  };
  func_positions_.set_number(func_num_range);

  if (program()->HasFunc(func_num)) {
    issue_tracker().Add(ir_issues::IssueKind::kDuplicateFuncNumber, func_num_range,
                        "@" + std::to_string(func_num) + " is already used.");
    return FuncNumberParseResult{
        .func_num = ir::kNoFuncNum,
        .range = func_num_range,
    };
  }
  return FuncNumberParseResult{
      .func_num = func_num,
      .range = func_num_range,
  };
}

// FuncArgs ::= '(' (Computed (',' Computed)*)? ')'
void FuncParser::ParseFuncArgs() {
  pos_t args_start = scanner().token_start();
  if (!scanner().ConsumeToken(Scanner::kParenOpen)) {
    return;
  }
  if (scanner().token() != Scanner::kParenClose) {
    ComputedValuesParseResult parsed_args = ParseComputedValues(/*expected_type=*/nullptr);
    for (const auto& arg : parsed_args.values) {
      func_->args().push_back(arg);
    }
    func_positions_.set_arg_ranges(parsed_args.value_ranges);
  }
  pos_t args_end = scanner().token_end();
  scanner().ConsumeToken(Scanner::kParenClose);
  func_positions_.set_args_range(range_t{
      .start = args_start,
      .end = args_end,
  });
}

// FuncResultTypes ::= '(' (Type (',' Type)*)? ')'
void FuncParser::ParseFuncResultTypes() {
  pos_t results_start = scanner().token_start();
  if (!scanner().ConsumeToken(Scanner::kParenOpen)) {
    return;
  }
  if (scanner().token() != Scanner::kParenClose) {
    TypeParser::TypesParseResult parsed_types = type_parser()->ParseTypes();
    for (const auto& result : parsed_types.types) {
      func_->result_types().push_back(result);
    }
    func_positions_.set_result_ranges(parsed_types.type_ranges);
  }
  pos_t results_end = scanner().token_end();
  scanner().ConsumeToken(Scanner::kParenClose);
  func_positions_.set_results_range(range_t{
      .start = results_start,
      .end = results_end,
  });
}

// FuncBody ::= '{' NL (NL | Block)* '}' NL
void FuncParser::ParseFuncBody() {
  pos_t body_start = scanner().token_start();
  pos_t body_end;
  scanner().ConsumeToken(Scanner::kCurlyBracketOpen);
  scanner().ConsumeToken(Scanner::kNewLine);

  for (;;) {
    if (scanner().token() == Scanner::kCurlyBracketClose) {
      body_end = scanner().token_end();
      scanner().ConsumeToken(Scanner::kCurlyBracketClose);
      break;
    } else if (scanner().token() == Scanner::kNewLine) {
      scanner().ConsumeToken(Scanner::kNewLine);
    } else if (scanner().token() == Scanner::kCurlyBracketOpen) {
      ParseBlock();
    } else {
      body_end = scanner().token_end();
      scanner().AddErrorForUnexpectedToken(
          {Scanner::kCurlyBracketOpen, Scanner::kCurlyBracketClose, Scanner::kNewLine});
      break;
    }
  }

  scanner().ConsumeToken(Scanner::kNewLine);
  ConnectBlocks();
  func_positions_.set_body(range_t{
      .start = body_start,
      .end = body_end,
  });
}

void FuncParser::ConnectBlocks() {
  for (auto& block : func_->blocks()) {
    if (block->instrs().empty()) {
      continue;
    }
    ir::Instr* last_instr = block->instrs().back().get();

    if (last_instr->instr_kind() == ir::InstrKind::kJump) {
      ir::JumpInstr* jump = static_cast<ir::JumpInstr*>(last_instr);
      ir::block_num_t child_num = jump->destination();
      const InstrPositions& jump_positions = program_positions_.GetInstrPositions(jump);

      if (!func_->HasBlock(child_num)) {
        issue_tracker_.Add(ir_issues::IssueKind::kUndefinedJumpDestination,
                           GetJumpInstrDestinationRange(jump_positions),
                           "{" + std::to_string(child_num) + "} does not exist");
      } else {
        func_->AddControlFlow(block->number(), child_num);
      }
    } else if (last_instr->instr_kind() == ir::InstrKind::kJumpCond) {
      ir::JumpCondInstr* jump_cond = static_cast<ir::JumpCondInstr*>(last_instr);
      ir::block_num_t child_a_num = jump_cond->destination_true();
      ir::block_num_t child_b_num = jump_cond->destination_false();
      const InstrPositions& jump_cond_positions = program_positions_.GetInstrPositions(jump_cond);

      if (!func_->HasBlock(child_a_num)) {
        issue_tracker_.Add(ir_issues::IssueKind::kUndefinedJumpDestination,
                           GetJumpCondInstrDestinationTrueRange(jump_cond_positions),
                           "{" + std::to_string(child_a_num) + "} does not exist");
      } else {
        func_->AddControlFlow(block->number(), child_a_num);
      }
      if (!func_->HasBlock(child_b_num)) {
        issue_tracker_.Add(ir_issues::IssueKind::kUndefinedJumpDestination,
                           GetJumpCondInstrDestinationFalseRange(jump_cond_positions),
                           "{" + std::to_string(child_b_num) + "} does not exist");
      } else {
        func_->AddControlFlow(block->number(), child_b_num);
      }
    }
  }
}

// Block ::= '{' Number '}' Identifier? NL Instr*
void FuncParser::ParseBlock() {
  ir::Block* block;
  BlockPositions block_positions;
  if (auto [block_num, block_num_range] = ParseBlockNumber(); block_num != ir::kNoBlockNum) {
    block = func_->AddBlock(block_num);
    block_positions.set_number(block_num_range);
    if (func_->entry_block() == nullptr) {
      func_->set_entry_block_num(block_num);
    }
  } else {
    return;
  }

  if (scanner().token() == Scanner::kIdentifier) {
    range_t name_range = scanner().token_range();
    block->set_name(scanner().ConsumeIdentifier().value());
    block_positions.set_name(name_range);
  }

  scanner().ConsumeToken(Scanner::kNewLine);
  ParseBlockBody(block, block_positions);

  program_positions_.AddBlockPositions(block, block_positions);
}

// BlockNum ::= '{' Number '}'
FuncParser::BlockNumberParseResult FuncParser::ParseBlockNumber() {
  pos_t block_num_start = scanner().token_start();
  if (!scanner().ConsumeToken(Scanner::kCurlyBracketOpen)) {
    scanner().SkipPastTokenSequence({Scanner::kNewLine, Scanner::kCurlyBracketClose});
    return BlockNumberParseResult{
        .block_num = ir::kNoBlockNum,
        .range = kNoRange,
    };
  }

  ir::block_num_t block_num = scanner().ConsumeInt64().value_or(ir::kNoBlockNum);
  if (block_num == ir::kNoBlockNum) {
    return BlockNumberParseResult{
        .block_num = ir::kNoBlockNum,
        .range = kNoRange,
    };
  }

  pos_t block_num_end = scanner().token_end();
  scanner().ConsumeToken(Scanner::kCurlyBracketClose);
  range_t block_num_range = range_t{
      .start = block_num_start,
      .end = block_num_end,
  };

  if (func_->HasBlock(block_num)) {
    issue_tracker().Add(ir_issues::IssueKind::kDuplicateBlockNumber, block_num_range,
                        "{" + std::to_string(block_num) + "} is already used.");
    return BlockNumberParseResult{
        .block_num = ir::kNoBlockNum,
        .range = block_num_range,
    };
  }
  return BlockNumberParseResult{
      .block_num = block_num,
      .range = block_num_range,
  };
}

void FuncParser::ParseBlockBody(ir::Block* block, BlockPositions& block_positions) {
  for (;;) {
    if (scanner().token() == Scanner::kCurlyBracketOpen ||
        scanner().token() == Scanner::kCurlyBracketClose) {
      break;
    } else if (scanner().token() == Scanner::kPercentSign ||
               scanner().token() == Scanner::kIdentifier) {
      std::unique_ptr<ir::Instr> instr = ParseInstr();
      if (instr != nullptr) {
        block->instrs().push_back(std::move(instr));
      }
    } else {
      scanner().AddErrorForUnexpectedToken({Scanner::kCurlyBracketOpen, Scanner::kCurlyBracketClose,
                                            Scanner::kPercentSign, Scanner::kNewLine});
      break;
    }
  }

  if (!block->instrs().empty()) {
    block_positions.set_body(range_t{
        .start = program_positions_.GetInstrPositions(block->instrs().front().get())
                     .entire_instr()
                     .start,
        .end =
            program_positions_.GetInstrPositions(block->instrs().front().get()).entire_instr().end,
    });
  }
}

// Instr ::= InstrResults '=' Idenifier (Value (',' Value)*)? NL
std::unique_ptr<ir::Instr> FuncParser::ParseInstr() {
  ComputedValuesParseResult parsed_results = ParseInstrResults();
  if (scanner().token() != Scanner::kIdentifier) {
    scanner().AddErrorForUnexpectedToken({Scanner::kIdentifier});
    scanner().SkipPastTokenSequence({Scanner::kNewLine});
    return nullptr;
  }
  range_t name_range = scanner().token_range();
  std::string name = scanner().ConsumeIdentifier().value();
  InstrParseResult parsed_instr = ParseInstrWithResults(parsed_results.values, name);
  const auto& instr = parsed_instr.instr;
  InstrPositions instr_positions;
  instr_positions.set_name(name_range);
  instr_positions.set_defined_value_ranges(parsed_results.value_ranges);
  instr_positions.set_used_value_ranges(parsed_instr.arg_ranges);

  program_positions_.AddInstrPositions(instr.get(), instr_positions);
  return std::move(parsed_instr).instr;
}

FuncParser::InstrParseResult FuncParser::NoInstrParseResult() {
  return InstrParseResult{
      .instr = nullptr,
      .arg_ranges = {},
      .args_range = kNoRange,
  };
}

// InstrWithResults ::= (Value (',' Value)*)? NL
FuncParser::InstrParseResult FuncParser::ParseInstrWithResults(
    std::vector<std::shared_ptr<ir::Computed>> results, std::string instr_name) {
  if (instr_name == "mov") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kMovInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for mov instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseMovInstr(results.front());

  } else if (instr_name == "phi") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPhiInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for phi instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParsePhiInstr(results.front());

  } else if (instr_name == "conv") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kConvInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for conv instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseConversionInstr(results.front());

  } else if (instr_name == "bnot") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kBoolNotInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for bool not instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseBoolNotInstr(results.front());

  } else if (auto bool_binary_op = common::atomics::ToBoolBinaryOp(instr_name); bool_binary_op) {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kBoolBinaryInstrDoesNotHaveOneResult,
                          scanner().token_start(),
                          "expected one result for bool binary instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseBoolBinaryInstr(results.front(), bool_binary_op.value());

  } else if (auto int_unary_op = common::atomics::ToIntUnaryOp(instr_name); int_unary_op) {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kIntUnaryInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for int unary instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseIntUnaryInstr(results.front(), int_unary_op.value());

  } else if (auto int_compare_op = common::atomics::ToIntCompareOp(instr_name); int_compare_op) {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kIntCompareInstrDoesNotHaveOneResult,
                          scanner().token_start(),
                          "expected one result for int compare instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseIntCompareInstr(results.front(), int_compare_op.value());

  } else if (auto int_binary_op = common::atomics::ToIntBinaryOp(instr_name); int_binary_op) {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kIntBinaryInstrDoesNotHaveOneResult,
                          scanner().token_start(),
                          "expected one result for int binary instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseIntBinaryInstr(results.front(), int_binary_op.value());

  } else if (auto int_shift_op = common::atomics::ToIntShiftOp(instr_name); int_shift_op) {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kIntShiftInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for int shift instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseIntShiftInstr(results.front(), int_shift_op.value());

  } else if (instr_name == "poff") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPointerOffsetInstrDoesNotHaveOneResult,
                          scanner().token_start(),
                          "expected one result for pointer offset instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParsePointerOffsetInstr(results.front());

  } else if (instr_name == "niltest") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kNilTestInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for nil test instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseNilTestInstr(results.front());

  } else if (instr_name == "malloc") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kMallocInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for malloc instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseMallocInstr(results.front());

  } else if (instr_name == "load") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kLoadInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for load instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseLoadInstr(results.front());

  } else if (instr_name == "store") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kStoreInstrHasResults, scanner().token_start(),
                          "did not expect results for store instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseStoreInstr();

  } else if (instr_name == "free") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kFreeInstrHasResults, scanner().token_start(),
                          "did not expect results for free instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseFreeInstr();

  } else if (instr_name == "jmp") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kJumpInstrHasResults, scanner().token_start(),
                          "did not expect results for jump instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseJumpInstr();

  } else if (instr_name == "jcc") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kJumpCondInstrHasResults, scanner().token_start(),
                          "did not expect results for jump conditional instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseJumpCondInstr();

  } else if (instr_name == "syscall") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kSyscallInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for syscall instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseSyscallInstr(results.front());

  } else if (instr_name == "call") {
    return ParseCallInstr(results);

  } else if (instr_name == "ret") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kReturnInstrHasResults, scanner().token_start(),
                          "did not expect results for return instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
    return ParseReturnInstr();

  } else {
    issue_tracker().Add(ir_issues::IssueKind::kUnknownInstructionName, scanner().token_start(),
                        "unknown instruction name");
    scanner().SkipPastTokenSequence({Scanner::kNewLine});
    return NoInstrParseResult();
  }
}

// MovInstr ::= Computed '=' 'mov' Value NL
FuncParser::InstrParseResult FuncParser::ParseMovInstr(std::shared_ptr<ir::Computed> result) {
  const auto& [arg, arg_range] = ParseValue(result->type());

  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::MovInstr>(result, arg),
      .arg_ranges = {arg_range},
      .args_range = arg_range,
  };
}

// PhiInstr ::= Computed '=' 'phi' InheritedValue (',' InheritedValue)+ NL
FuncParser::InstrParseResult FuncParser::ParsePhiInstr(std::shared_ptr<ir::Computed> result) {
  const auto& [first_arg, first_arg_range] = ParseInheritedValue(result->type());
  std::vector<std::shared_ptr<ir::InheritedValue>> args{first_arg};
  std::vector<range_t> arg_ranges{first_arg_range};

  for (;;) {
    if (scanner().token() == Scanner::kNewLine) {
      scanner().ConsumeToken(Scanner::kNewLine);
      break;
    } else if (scanner().token() == Scanner::kComma) {
      scanner().ConsumeToken(Scanner::kComma);

      const auto& [arg, arg_range] = ParseInheritedValue(result->type());
      args.push_back(arg);
      arg_ranges.push_back(arg_range);

    } else {
      scanner().AddErrorForUnexpectedToken({Scanner::kNewLine, Scanner::kComma});
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return NoInstrParseResult();
    }
  }

  if (args.size() < 2) {
    issue_tracker().Add(ir_issues::IssueKind::kPhiInstrHasLessThanTwoResults,
                        scanner().token_start(),
                        "expected at least two arguments for phi instruction");
  }

  return InstrParseResult{
      .instr = std::make_unique<ir::PhiInstr>(result, args),
      .arg_ranges = arg_ranges,
      .args_range =
          range_t{
              .start = arg_ranges.front().start,
              .end = arg_ranges.back().end,
          },
  };
}

// MovInstr ::= Computed '=' 'conv' Value NL
FuncParser::InstrParseResult FuncParser::ParseConversionInstr(
    std::shared_ptr<ir::Computed> result) {
  const auto& [arg, arg_range] = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::Conversion>(result, arg),
      .arg_ranges = {arg_range},
      .args_range = arg_range,
  };
}

// BoolNotInstr ::= Computed '=' 'bnot' Value NL
FuncParser::InstrParseResult FuncParser::ParseBoolNotInstr(std::shared_ptr<ir::Computed> result) {
  const auto& [operand, operand_range] = ParseValue(ir::bool_type());

  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::BoolNotInstr>(result, operand),
      .arg_ranges = {operand_range},
      .args_range = operand_range,
  };
}

// BoolBinaryInstr ::= Computed '=' BinaryOp Value ',' Value NL
FuncParser::InstrParseResult FuncParser::ParseBoolBinaryInstr(std::shared_ptr<ir::Computed> result,
                                                              Bool::BinaryOp op) {
  const auto& [operand_a, operand_a_range] = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kComma);

  const auto& [operand_b, operand_b_range] = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::BoolBinaryInstr>(result, op, operand_a, operand_b),
      .arg_ranges = {operand_a_range, operand_b_range},
      .args_range =
          range_t{
              .start = operand_a_range.start,
              .end = operand_b_range.end,
          },
  };
}

// IntUnaryInstr ::= Computed '=' UnaryOp Value NL
FuncParser::InstrParseResult FuncParser::ParseIntUnaryInstr(std::shared_ptr<ir::Computed> result,
                                                            Int::UnaryOp op) {
  const auto& [operand, operand_range] = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::IntUnaryInstr>(result, op, operand),
      .arg_ranges = {operand_range},
      .args_range = operand_range,
  };
}

// IntCompareInstr ::= Computed '=' CompareOp Value ',' Value NL
FuncParser::InstrParseResult FuncParser::ParseIntCompareInstr(std::shared_ptr<ir::Computed> result,
                                                              Int::CompareOp op) {
  const auto& [operand_a, operand_a_range] = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kComma);

  const auto& [operand_b, operand_b_range] =
      ParseValue(operand_a != nullptr ? operand_a->type() : nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::IntCompareInstr>(result, op, operand_a, operand_b),
      .arg_ranges = {operand_a_range, operand_b_range},
      .args_range =
          range_t{
              .start = operand_a_range.start,
              .end = operand_b_range.end,
          },
  };
}

// IntBinaryInstr ::= Computed '=' BinaryOp Value ',' Value NL
FuncParser::InstrParseResult FuncParser::ParseIntBinaryInstr(std::shared_ptr<ir::Computed> result,
                                                             Int::BinaryOp op) {
  const auto& [operand_a, operand_a_range] = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kComma);

  const auto& [operand_b, operand_b_range] = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::IntBinaryInstr>(result, op, operand_a, operand_b),
      .arg_ranges = {operand_a_range, operand_b_range},
      .args_range =
          range_t{
              .start = operand_a_range.start,
              .end = operand_b_range.end,
          },
  };
}

// IntShiftInstr ::= Computed '=' ShiftOp Value ',' Value NL
FuncParser::InstrParseResult FuncParser::ParseIntShiftInstr(std::shared_ptr<ir::Computed> result,
                                                            Int::ShiftOp op) {
  const auto& [operand_a, operand_a_range] = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kComma);

  const auto& [operand_b, operand_b_range] = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::IntShiftInstr>(result, op, operand_a, operand_b),
      .arg_ranges = {operand_a_range, operand_b_range},
      .args_range =
          range_t{
              .start = operand_a_range.start,
              .end = operand_b_range.end,
          },
  };
}

// PointerOffsetInstr ::= Computed '=' 'poff' Value ',' Value NL
FuncParser::InstrParseResult FuncParser::ParsePointerOffsetInstr(
    std::shared_ptr<ir::Computed> result) {
  const auto& [pointer, pointer_range] = ParseComputedValue(ir::pointer_type());
  scanner().ConsumeToken(Scanner::kComma);

  const auto& [offset, offset_range] = ParseValue(ir::i64());
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::PointerOffsetInstr>(result, pointer, offset),
      .arg_ranges = {pointer_range, offset_range},
      .args_range =
          range_t{
              .start = pointer_range.start,
              .end = offset_range.end,
          },
  };
}

// NilTestInstr ::= Computed '=' 'niltest' Value NL
FuncParser::InstrParseResult FuncParser::ParseNilTestInstr(std::shared_ptr<ir::Computed> result) {
  const auto& [tested, tested_range] = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::NilTestInstr>(result, tested),
      .arg_ranges = {tested_range},
      .args_range = tested_range,
  };
}

// MallocInstr ::= Comptued '=' 'malloc' Value NL
FuncParser::InstrParseResult FuncParser::ParseMallocInstr(std::shared_ptr<ir::Computed> result) {
  const auto& [size, size_range] = ParseValue(ir::i64());
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::MallocInstr>(result, size),
      .arg_ranges = {size_range},
      .args_range = size_range,
  };
}

// LoadInstr ::= Computed '=' 'load' Value NL
FuncParser::InstrParseResult FuncParser::ParseLoadInstr(std::shared_ptr<ir::Computed> result) {
  const auto& [address, address_range] = ParseValue(ir::pointer_type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::LoadInstr>(result, address),
      .arg_ranges = {address_range},
      .args_range = address_range,
  };
}

// StoreInstr ::= 'store' Value ',' Value NL
FuncParser::InstrParseResult FuncParser::ParseStoreInstr() {
  const auto& [address, address_range] = ParseValue(ir::pointer_type());
  scanner().ConsumeToken(Scanner::kComma);

  const auto& [value, value_range] = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::StoreInstr>(address, value),
      .arg_ranges = {address_range, value_range},
      .args_range =
          range_t{
              .start = address_range.start,
              .end = value_range.end,
          },
  };
}

// FreeInstr ::= 'free' Value NL
FuncParser::InstrParseResult FuncParser::ParseFreeInstr() {
  const auto& [address, address_range] = ParseValue(ir::pointer_type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::FreeInstr>(address),
      .arg_ranges = {address_range},
      .args_range = address_range,
  };
}

// JumpInstr ::= 'jmp' BlockValue NL
FuncParser::InstrParseResult FuncParser::ParseJumpInstr() {
  const auto& [destination, destination_range] = ParseBlockValue();
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::JumpInstr>(destination),
      .arg_ranges = {destination_range},
      .args_range = destination_range,
  };
}

// JumpCondInstr ::= 'jcc' Value ',' BlockValue ',' BlockValue NL
FuncParser::InstrParseResult FuncParser::ParseJumpCondInstr() {
  const auto& [condition, condition_range] = ParseValue(ir::bool_type());
  scanner().ConsumeToken(Scanner::kComma);

  const auto& [destination_true, destination_true_range] = ParseBlockValue();
  scanner().ConsumeToken(Scanner::kComma);

  const auto& [destination_false, destination_false_range] = ParseBlockValue();
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::JumpCondInstr>(condition, destination_true, destination_false),
      .arg_ranges = {condition_range, destination_true_range, destination_false_range},
      .args_range =
          range_t{
              .start = condition_range.start,
              .end = destination_false_range.end,
          },
  };
}

// SyscallInstr ::= Computed '=' 'syscall' Value (',' Value)* NL
FuncParser::InstrParseResult FuncParser::ParseSyscallInstr(std::shared_ptr<ir::Computed> result) {
  const auto& [syscall_num, syscall_num_range] = ParseValue(ir::i64());
  std::vector<std::shared_ptr<ir::Value>> args;
  std::vector<range_t> arg_ranges;
  if (scanner().token() == Scanner::kComma) {
    scanner().ConsumeToken(Scanner::kComma);
    ValuesParseResult parsed_args = ParseValues(/*expected_type=*/ir::i64());
    args = parsed_args.values;
    arg_ranges = parsed_args.value_ranges;
  }
  arg_ranges.insert(arg_ranges.begin(), syscall_num_range);
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::SyscallInstr>(result, syscall_num, args),
      .arg_ranges = arg_ranges,
      .args_range =
          range_t{
              .start = syscall_num_range.start,
              .end = arg_ranges.back().end,
          },
  };
}

// CallInstr ::= (Computed (',' Computed)* '=')?
//               'call' Value (',' Value)* NL
FuncParser::InstrParseResult FuncParser::ParseCallInstr(
    std::vector<std::shared_ptr<ir::Computed>> results) {
  const auto& [func, func_range] = ParseValue(ir::func_type());
  std::vector<std::shared_ptr<ir::Value>> args;
  std::vector<range_t> arg_ranges;
  if (scanner().token() == Scanner::kComma) {
    scanner().ConsumeToken(Scanner::kComma);
    ValuesParseResult parsed_args = ParseValues(/*expected_type=*/nullptr);
    args = parsed_args.values;
    arg_ranges = parsed_args.value_ranges;
  }
  arg_ranges.insert(arg_ranges.begin(), func_range);
  scanner().ConsumeToken(Scanner::kNewLine);

  return InstrParseResult{
      .instr = std::make_unique<ir::CallInstr>(func, results, args),
      .arg_ranges = arg_ranges,
      .args_range =
          range_t{
              .start = func_range.start,
              .end = arg_ranges.back().end,
          },
  };
}

FuncParser::InstrParseResult FuncParser::ParseReturnInstr() {
  if (scanner().token() == Scanner::kNewLine) {
    scanner().ConsumeToken(Scanner::kNewLine);
    return InstrParseResult{
        .instr =
            std::make_unique<ir::ReturnInstr>(/*args=*/std::vector<std::shared_ptr<ir::Value>>{}),
        .arg_ranges = {},
        .args_range = kNoRange,
    };
  }
  ValuesParseResult parsed_args = ParseValues(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);
  return InstrParseResult{
      .instr = std::make_unique<ir::ReturnInstr>(parsed_args.values),
      .arg_ranges = parsed_args.value_ranges,
      .args_range = parsed_args.range,
  };
}

// InstrResults ::= (Computed (',' Computed)* '=')?
FuncParser::ComputedValuesParseResult FuncParser::ParseInstrResults() {
  if (scanner().token() == Scanner::kPercentSign) {
    ComputedValuesParseResult result = ParseComputedValues(/*expected_type=*/nullptr);
    scanner().ConsumeToken(Scanner::kEqualSign);
    return result;
  } else {
    return ComputedValuesParseResult{
        .values = {},
        .value_ranges = {},
        .range = kNoRange,
    };
  }
}

// InheritedValue ::= (Constant | Computed) BlockValue
FuncParser::InheritedValueParseResult FuncParser::ParseInheritedValue(
    const ir::Type* expected_type) {
  const auto& [value, value_range] = ParseValue(expected_type);
  const auto& [origin, origin_range] = ParseBlockValue();
  return InheritedValueParseResult{
      .value = std::make_shared<ir::InheritedValue>(value, origin),
      .range =
          range_t{
              .start = value_range.start,
              .end = origin_range.end,
          },
  };
}

// Values ::= Value (',' Value)*
FuncParser::ValuesParseResult FuncParser::ParseValues(const ir::Type* expected_type) {
  const auto& [first_value, first_range] = ParseValue(expected_type);
  std::vector<std::shared_ptr<ir::Value>> values{first_value};
  std::vector<range_t> ranges{first_range};
  while (scanner().token() == Scanner::kComma) {
    scanner().ConsumeToken(Scanner::kComma);
    const auto& [value, range] = ParseValue(expected_type);
    values.push_back(value);
    ranges.push_back(range);
  }
  return ValuesParseResult{
      .values = values,
      .value_ranges = ranges,
      .range =
          range_t{
              .start = ranges.front().start,
              .end = ranges.back().end,
          },
  };
}

// Value ::= (Constant | Computed)
FuncParser::ValueParseResult FuncParser::ParseValue(const ir::Type* expected_type) {
  if (scanner().token() == Scanner::kPercentSign) {
    const auto& [value, range] = ParseComputedValue(expected_type);
    return ValueParseResult{
        .value = value,
        .range = range,
    };
  } else {
    const auto& [value, range] = constant_parser()->ParseConstant(expected_type);
    return ValueParseResult{
        .value = value,
        .range = range,
    };
  }
}

// Computed values ::= Computed (',' Computed)*
FuncParser::ComputedValuesParseResult FuncParser::ParseComputedValues(
    const ir::Type* expected_type) {
  const auto& [first_value, first_range] = ParseComputedValue(expected_type);
  std::vector<std::shared_ptr<ir::Computed>> values{first_value};
  std::vector<range_t> ranges{first_range};
  while (scanner().token() == Scanner::kComma) {
    scanner().ConsumeToken(Scanner::kComma);
    const auto& [value, range] = ParseComputedValue(expected_type);
    values.push_back(value);
    ranges.push_back(range);
  }
  return ComputedValuesParseResult{
      .values = values,
      .value_ranges = ranges,
      .range =
          range_t{
              .start = ranges.front().start,
              .end = ranges.back().end,
          },
  };
}

// Computed ::= '%' Identifier (':' Type)?
FuncParser::ComputedValueParseResult FuncParser::ParseComputedValue(const ir::Type* expected_type) {
  pos_t start = scanner().token_start();
  scanner().ConsumeToken(Scanner::kPercentSign);
  pos_t end = scanner().token_end();
  ir::value_num_t number = scanner().ConsumeInt64().value_or(ir::kNoValueNum);
  bool known = computed_values_.contains(number);

  const ir::Type* type;
  if (scanner().token() == Scanner::kColon) {
    scanner().ConsumeToken(Scanner::kColon);
    TypeParser::TypeParseResult parsed_type = type_parser()->ParseType();
    type = parsed_type.type;
    end = parsed_type.range.end;

    if (type == nullptr) {
      type = expected_type;
    } else if (expected_type != nullptr && expected_type != type) {
      issue_tracker().Add(
          ir_issues::IssueKind::kUnexpectedType, parsed_type.range,
          "expected '" + expected_type->RefString() + "'; got '" + type->RefString() + "'");
      type = expected_type;
    }
  } else {
    if (expected_type == nullptr && !known) {
      scanner().AddErrorForUnexpectedToken({Scanner::kColon});
    }
    type = expected_type;
  }

  std::shared_ptr<ir::Computed> computed;
  if (known) {
    computed = computed_values_.at(number);
  } else {
    computed = std::make_shared<ir::Computed>(type, number);
    func_->register_computed_number(number);
    computed_values_.insert({number, computed});
  }
  return ComputedValueParseResult{
      .value = computed,
      .range =
          range_t{
              .start = start,
              .end = end,
          },
  };
}

// BlockValue ::= '{' Number '}'
FuncParser::BlockValueParseResult FuncParser::ParseBlockValue() {
  pos_t start = scanner().token_start();
  scanner().ConsumeToken(Scanner::kCurlyBracketOpen);
  ir::block_num_t number = scanner().ConsumeInt64().value_or(ir::kNoBlockNum);
  pos_t end = scanner().token_end();
  scanner().ConsumeToken(Scanner::kCurlyBracketClose);
  return BlockValueParseResult{
      .value = number,
      .range =
          range_t{
              .start = start,
              .end = end,
          },
  };
}

}  // namespace ir_serialization

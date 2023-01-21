//
//  parser.cc
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "func_parser.h"

namespace ir_serialization {

using ::common::atomics::Bool;
using ::common::atomics::Int;
using ::common::positions::pos_t;

// Func ::= '@' Number Identifier? FuncArgs '=>' FuncResultTypes FuncBody
ir::Func* FuncParser::ParseFunc() {
  pos_t func_start = scanner().token_start();
  if (!scanner().ConsumeToken(Scanner::kAtSign)) {
    scanner().SkipPastTokenSequence({Scanner::kNewLine, Scanner::kCurlyBracketClose});
    return nullptr;
  }

  pos_t func_num_pos = scanner().token_start();
  ir::func_num_t func_num = scanner().ConsumeInt64().value_or(ir::kNoFuncNum);
  if (func_num != ir::kNoFuncNum) {
    func_num += func_num_offset_;
  }
  if (func_num != ir::kNoFuncNum && program()->HasFunc(func_num)) {
    issue_tracker().Add(ir_issues::IssueKind::kDuplicateFuncNumber, func_num_pos,
                        "@" + std::to_string(func_num) + " is already used.");
    func_num = ir::kNoFuncNum;
  }
  func_ = program()->AddFunc(func_num);

  if (scanner().token() == Scanner::kIdentifier) {
    func_->set_name(scanner().ConsumeIdentifier().value_or(""));
    if (func_->name() == "main") {
      program_->set_entry_func_num(func_->number());
    }
  }

  ParseFuncArgs();
  scanner().ConsumeToken(Scanner::kArrow);
  ParseFuncResultTypes();
  pos_t func_end = ParseFuncBody();
  func_->SetPositions(func_start, func_end);
  return func_;
}

// FuncArgs ::= '(' (Computed (',' Computed)*)? ')'
void FuncParser::ParseFuncArgs() {
  if (!scanner().ConsumeToken(Scanner::kParenOpen)) {
    return;
  }
  if (scanner().token() != Scanner::kParenClose) {
    func_->args() = ParseComputedValues(/*expected_type=*/nullptr);
  }
  scanner().ConsumeToken(Scanner::kParenClose);
}

// FuncResultTypes ::= '(' (Type (',' Type)*)? ')'
void FuncParser::ParseFuncResultTypes() {
  if (!scanner().ConsumeToken(Scanner::kParenOpen)) {
    return;
  }
  if (scanner().token() != Scanner::kParenClose) {
    func_->result_types() = type_parser()->ParseTypes();
  }
  scanner().ConsumeToken(Scanner::kParenClose);
}

// FuncBody ::= '{' NL (NL | Block)* '}' NL
pos_t FuncParser::ParseFuncBody() {
  pos_t func_end;
  scanner().ConsumeToken(Scanner::kCurlyBracketOpen);
  scanner().ConsumeToken(Scanner::kNewLine);

  for (;;) {
    if (scanner().token() == Scanner::kCurlyBracketClose) {
      func_end = scanner().token_end();
      scanner().ConsumeToken(Scanner::kCurlyBracketClose);
      break;
    } else if (scanner().token() == Scanner::kNewLine) {
      scanner().ConsumeToken(Scanner::kNewLine);
    } else if (scanner().token() == Scanner::kCurlyBracketOpen) {
      ParseBlock();
    } else {
      func_end = scanner().token_end();
      scanner().AddErrorForUnexpectedToken(
          {Scanner::kCurlyBracketOpen, Scanner::kCurlyBracketClose, Scanner::kNewLine});
      break;
    }
  }

  scanner().ConsumeToken(Scanner::kNewLine);
  ConnectBlocks();
  return func_end;
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

      if (!func_->HasBlock(child_num)) {
        issue_tracker_.Add(ir_issues::IssueKind::kUndefinedJumpDestination, jump->start(),
                           "{" + std::to_string(child_num) + "} does not exist");
        return;
      } else {
        func_->AddControlFlow(block->number(), child_num);
      }
    } else if (last_instr->instr_kind() == ir::InstrKind::kJumpCond) {
      ir::JumpCondInstr* jump_cond = static_cast<ir::JumpCondInstr*>(last_instr);
      ir::block_num_t child_a_num = jump_cond->destination_true();
      ir::block_num_t child_b_num = jump_cond->destination_false();

      if (!func_->HasBlock(child_a_num)) {
        issue_tracker_.Add(ir_issues::IssueKind::kUndefinedJumpDestination, jump_cond->start(),
                           "{" + std::to_string(child_a_num) + "} does not exist");
      } else {
        func_->AddControlFlow(block->number(), child_a_num);
      }
      if (!func_->HasBlock(child_b_num)) {
        issue_tracker_.Add(ir_issues::IssueKind::kUndefinedJumpDestination, jump_cond->start(),
                           "{" + std::to_string(child_b_num) + "} does not exist");
      } else {
        func_->AddControlFlow(block->number(), child_b_num);
      }
    }
  }
}

// Block ::= '{' Number '}' Identifier? NL Instr*
void FuncParser::ParseBlock() {
  pos_t block_start = scanner().token_start();
  scanner().ConsumeToken(Scanner::kCurlyBracketOpen);

  pos_t block_num_pos = scanner().token_start();
  ir::block_num_t block_num = scanner().ConsumeInt64().value_or(ir::kNoBlockNum);
  if (block_num != ir::kNoBlockNum && func_->HasBlock(block_num)) {
    issue_tracker().Add(ir_issues::IssueKind::kDuplicateBlockNumber, block_num_pos,
                        "{" + std::to_string(block_num) + "} is already used.");
    block_num = ir::kNoBlockNum;
  }

  scanner().ConsumeToken(Scanner::kCurlyBracketClose);

  ir::Block* block = func_->AddBlock(block_num);
  if (func_->entry_block() == nullptr) {
    func_->set_entry_block_num(block_num);
  }
  if (scanner().token() == Scanner::kIdentifier) {
    block->set_name(scanner().ConsumeIdentifier().value());
  }

  scanner().ConsumeToken(Scanner::kNewLine);

  pos_t block_end = scanner().token_start() - 1;
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
      block_end = scanner().token_start() - 1;
    } else {
      scanner().AddErrorForUnexpectedToken({Scanner::kCurlyBracketOpen, Scanner::kCurlyBracketClose,
                                            Scanner::kPercentSign, Scanner::kNewLine});
      break;
    }
  }
  block->SetPositions(block_start, block_end);
}

// Instr ::= InstrResults '=' Idenifier (Value (',' Value)*)? NL
std::unique_ptr<ir::Instr> FuncParser::ParseInstr() {
  pos_t instr_start = scanner().token_start();
  std::vector<std::shared_ptr<ir::Computed>> results = ParseInstrResults();
  if (scanner().token() != Scanner::kIdentifier) {
    scanner().AddErrorForUnexpectedToken({Scanner::kIdentifier});
    scanner().SkipPastTokenSequence({Scanner::kNewLine});
    return nullptr;
  }
  std::string instr_name = scanner().ConsumeIdentifier().value();
  std::unique_ptr<ir::Instr> instr = ParseInstrWithResults(results, instr_name);
  pos_t instr_end = scanner().token_start() - 1;
  instr->SetPositions(instr_start, instr_end);
  return instr;
}

// InstrWithResults ::= (Value (',' Value)*)? NL
std::unique_ptr<ir::Instr> FuncParser::ParseInstrWithResults(
    std::vector<std::shared_ptr<ir::Computed>> results, std::string instr_name) {
  if (instr_name == "mov") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kMovInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for mov instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseMovInstr(results.front());

  } else if (instr_name == "phi") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPhiInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for phi instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParsePhiInstr(results.front());

  } else if (instr_name == "conv") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kConvInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for conv instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseConversionInstr(results.front());

  } else if (instr_name == "bnot") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kBoolNotInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for bool not instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseBoolNotInstr(results.front());

  } else if (auto bool_binary_op = common::atomics::ToBoolBinaryOp(instr_name); bool_binary_op) {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kBoolBinaryInstrDoesNotHaveOneResult,
                          scanner().token_start(),
                          "expected one result for bool binary instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseBoolBinaryInstr(results.front(), bool_binary_op.value());

  } else if (auto int_unary_op = common::atomics::ToIntUnaryOp(instr_name); int_unary_op) {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kIntUnaryInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for int unary instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseIntUnaryInstr(results.front(), int_unary_op.value());

  } else if (auto int_compare_op = common::atomics::ToIntCompareOp(instr_name); int_compare_op) {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kIntCompareInstrDoesNotHaveOneResult,
                          scanner().token_start(),
                          "expected one result for int compare instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseIntCompareInstr(results.front(), int_compare_op.value());

  } else if (auto int_binary_op = common::atomics::ToIntBinaryOp(instr_name); int_binary_op) {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kIntBinaryInstrDoesNotHaveOneResult,
                          scanner().token_start(),
                          "expected one result for int binary instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseIntBinaryInstr(results.front(), int_binary_op.value());

  } else if (auto int_shift_op = common::atomics::ToIntShiftOp(instr_name); int_shift_op) {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kIntShiftInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for int shift instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseIntShiftInstr(results.front(), int_shift_op.value());

  } else if (instr_name == "poff") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kPointerOffsetInstrDoesNotHaveOneResult,
                          scanner().token_start(),
                          "expected one result for pointer offset instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParsePointerOffsetInstr(results.front());

  } else if (instr_name == "niltest") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kNilTestInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for nil test instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseNilTestInstr(results.front());

  } else if (instr_name == "malloc") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kMallocInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for malloc instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseMallocInstr(results.front());

  } else if (instr_name == "load") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kLoadInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for load instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseLoadInstr(results.front());

  } else if (instr_name == "store") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kStoreInstrHasResults, scanner().token_start(),
                          "did not expect results for store instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseStoreInstr();

  } else if (instr_name == "free") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kFreeInstrHasResults, scanner().token_start(),
                          "did not expect results for free instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseFreeInstr();

  } else if (instr_name == "jmp") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kJumpInstrHasResults, scanner().token_start(),
                          "did not expect results for jump instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseJumpInstr();

  } else if (instr_name == "jcc") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kJumpCondInstrHasResults, scanner().token_start(),
                          "did not expect results for jump conditional instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseJumpCondInstr();

  } else if (instr_name == "syscall") {
    if (results.size() != 1) {
      issue_tracker().Add(ir_issues::IssueKind::kSyscallInstrDoesNotHaveOneResult,
                          scanner().token_start(), "expected one result for syscall instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseSyscallInstr(results.front());

  } else if (instr_name == "call") {
    return ParseCallInstr(results);

  } else if (instr_name == "ret") {
    if (results.size() != 0) {
      issue_tracker().Add(ir_issues::IssueKind::kReturnInstrHasResults, scanner().token_start(),
                          "did not expect results for return instruction");
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
    return ParseReturnInstr();

  } else {
    issue_tracker().Add(ir_issues::IssueKind::kUnknownInstructionName, scanner().token_start(),
                        "unknown instruction name");
    scanner().SkipPastTokenSequence({Scanner::kNewLine});
    return nullptr;
  }
}

// MovInstr ::= Computed '=' 'mov' Value NL
std::unique_ptr<ir::MovInstr> FuncParser::ParseMovInstr(std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> arg = ParseValue(result->type());

  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::MovInstr>(result, arg);
}

// PhiInstr ::= Computed '=' 'phi' InheritedValue (',' InheritedValue)+ NL
std::unique_ptr<ir::PhiInstr> FuncParser::ParsePhiInstr(std::shared_ptr<ir::Computed> result) {
  std::vector<std::shared_ptr<ir::InheritedValue>> args;

  args.push_back(ParseInheritedValue(result->type()));

  for (;;) {
    if (scanner().token() == Scanner::kNewLine) {
      scanner().ConsumeToken(Scanner::kNewLine);
      break;
    } else if (scanner().token() == Scanner::kComma) {
      scanner().ConsumeToken(Scanner::kComma);

      args.push_back(ParseInheritedValue(result->type()));

    } else {
      scanner().AddErrorForUnexpectedToken({Scanner::kNewLine, Scanner::kComma});
      scanner().SkipPastTokenSequence({Scanner::kNewLine});
      return nullptr;
    }
  }

  if (args.size() < 2) {
    issue_tracker().Add(ir_issues::IssueKind::kPhiInstrHasLessThanTwoResults,
                        scanner().token_start(),
                        "expected at least two arguments for phi instruction");
  }

  return std::make_unique<ir::PhiInstr>(result, args);
}

// MovInstr ::= Computed '=' 'conv' Value NL
std::unique_ptr<ir::Conversion> FuncParser::ParseConversionInstr(
    std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> arg = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::Conversion>(result, arg);
}

// BoolNotInstr ::= Computed '=' 'bnot' Value NL
std::unique_ptr<ir::BoolNotInstr> FuncParser::ParseBoolNotInstr(
    std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> operand = ParseValue(ir::bool_type());

  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::BoolNotInstr>(result, operand);
}

// BoolBinaryInstr ::= Computed '=' BinaryOp Value ',' Value NL
std::unique_ptr<ir::BoolBinaryInstr> FuncParser::ParseBoolBinaryInstr(
    std::shared_ptr<ir::Computed> result, Bool::BinaryOp op) {
  std::shared_ptr<ir::Value> operand_a = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kComma);

  std::shared_ptr<ir::Value> operand_b = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::BoolBinaryInstr>(result, op, operand_a, operand_b);
}

// IntUnaryInstr ::= Computed '=' UnaryOp Value NL
std::unique_ptr<ir::IntUnaryInstr> FuncParser::ParseIntUnaryInstr(
    std::shared_ptr<ir::Computed> result, Int::UnaryOp op) {
  std::shared_ptr<ir::Value> operand = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::IntUnaryInstr>(result, op, operand);
}

// IntCompareInstr ::= Computed '=' CompareOp Value ',' Value NL
std::unique_ptr<ir::IntCompareInstr> FuncParser::ParseIntCompareInstr(
    std::shared_ptr<ir::Computed> result, Int::CompareOp op) {
  std::shared_ptr<ir::Value> operand_a = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kComma);

  std::shared_ptr<ir::Value> operand_b =
      ParseValue(operand_a != nullptr ? operand_a->type() : nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::IntCompareInstr>(result, op, operand_a, operand_b);
}

// IntBinaryInstr ::= Computed '=' BinaryOp Value ',' Value NL
std::unique_ptr<ir::IntBinaryInstr> FuncParser::ParseIntBinaryInstr(
    std::shared_ptr<ir::Computed> result, Int::BinaryOp op) {
  std::shared_ptr<ir::Value> operand_a = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kComma);

  std::shared_ptr<ir::Value> operand_b = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::IntBinaryInstr>(result, op, operand_a, operand_b);
}

// IntShiftInstr ::= Computed '=' ShiftOp Value ',' Value NL
std::unique_ptr<ir::IntShiftInstr> FuncParser::ParseIntShiftInstr(
    std::shared_ptr<ir::Computed> result, Int::ShiftOp op) {
  std::shared_ptr<ir::Value> operand_a = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kComma);

  std::shared_ptr<ir::Value> operand_b = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::IntShiftInstr>(result, op, operand_a, operand_b);
}

// PointerOffsetInstr ::= Computed '=' 'poff' Value ',' Value NL
std::unique_ptr<ir::PointerOffsetInstr> FuncParser::ParsePointerOffsetInstr(
    std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Computed> pointer = ParseComputedValue(ir::pointer_type());
  scanner().ConsumeToken(Scanner::kComma);

  std::shared_ptr<ir::Value> offset = ParseValue(ir::i64());
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::PointerOffsetInstr>(result, pointer, offset);
}

// NilTestInstr ::= Computed '=' 'niltest' Value NL
std::unique_ptr<ir::NilTestInstr> FuncParser::ParseNilTestInstr(
    std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> tested = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::NilTestInstr>(result, tested);
}

// MallocInstr ::= Comptued '=' 'malloc' Value NL
std::unique_ptr<ir::MallocInstr> FuncParser::ParseMallocInstr(
    std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> size = ParseValue(ir::i64());
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::MallocInstr>(result, size);
}

// LoadInstr ::= Computed '=' 'load' Value NL
std::unique_ptr<ir::LoadInstr> FuncParser::ParseLoadInstr(std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> address = ParseValue(ir::pointer_type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::LoadInstr>(result, address);
}

// StoreInstr ::= 'store' Value ',' Value NL
std::unique_ptr<ir::StoreInstr> FuncParser::ParseStoreInstr() {
  std::shared_ptr<ir::Value> address = ParseValue(ir::pointer_type());
  scanner().ConsumeToken(Scanner::kComma);

  std::shared_ptr<ir::Value> value = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::StoreInstr>(address, value);
}

// FreeInstr ::= 'free' Value NL
std::unique_ptr<ir::FreeInstr> FuncParser::ParseFreeInstr() {
  std::shared_ptr<ir::Value> address = ParseValue(ir::pointer_type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::FreeInstr>(address);
}

// JumpInstr ::= 'jmp' BlockValue NL
std::unique_ptr<ir::JumpInstr> FuncParser::ParseJumpInstr() {
  ir::block_num_t destination = ParseBlockValue();
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::JumpInstr>(destination);
}

// JumpCondInstr ::= 'jcc' Value ',' BlockValue ',' BlockValue NL
std::unique_ptr<ir::JumpCondInstr> FuncParser::ParseJumpCondInstr() {
  std::shared_ptr<ir::Value> condition = ParseValue(ir::bool_type());
  scanner().ConsumeToken(Scanner::kComma);

  ir::block_num_t destination_true = ParseBlockValue();
  scanner().ConsumeToken(Scanner::kComma);

  ir::block_num_t destination_false = ParseBlockValue();
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::JumpCondInstr>(condition, destination_true, destination_false);
}

// SyscallInstr ::= Computed '=' 'syscall' Value (',' Value)* NL
std::unique_ptr<ir::SyscallInstr> FuncParser::ParseSyscallInstr(
    std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> syscall_num = ParseValue(ir::i64());
  std::vector<std::shared_ptr<ir::Value>> args;
  if (scanner().token() == Scanner::kComma) {
    scanner().ConsumeToken(Scanner::kComma);
    args = ParseValues(/*expected_type=*/ir::i64());
  }
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::SyscallInstr>(result, syscall_num, args);
}

// CallInstr ::= (Computed (',' Computed)* '=')?
//               'call' Value (',' Value)* NL
std::unique_ptr<ir::CallInstr> FuncParser::ParseCallInstr(
    std::vector<std::shared_ptr<ir::Computed>> results) {
  std::shared_ptr<ir::Value> func = ParseValue(ir::func_type());
  std::vector<std::shared_ptr<ir::Value>> args;
  if (scanner().token() == Scanner::kComma) {
    scanner().ConsumeToken(Scanner::kComma);
    args = ParseValues(/*expected_type=*/nullptr);
  }
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::CallInstr>(func, results, args);
}

std::unique_ptr<ir::ReturnInstr> FuncParser::ParseReturnInstr() {
  if (scanner().token() == Scanner::kNewLine) {
    scanner().ConsumeToken(Scanner::kNewLine);
    return std::make_unique<ir::ReturnInstr>(/*args=*/std::vector<std::shared_ptr<ir::Value>>{});
  }
  std::vector<std::shared_ptr<ir::Value>> args = ParseValues(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);
  return std::make_unique<ir::ReturnInstr>(args);
}

// InstrResults ::= (Computed (',' Computed)* '=')?
std::vector<std::shared_ptr<ir::Computed>> FuncParser::ParseInstrResults() {
  std::vector<std::shared_ptr<ir::Computed>> results;
  if (scanner().token() == Scanner::kPercentSign) {
    results = ParseComputedValues(/*expected_type=*/nullptr);
    scanner().ConsumeToken(Scanner::kEqualSign);
  }
  return results;
}

// InheritedValue ::= (Constant | Computed) BlockValue
std::shared_ptr<ir::InheritedValue> FuncParser::ParseInheritedValue(const ir::Type* expected_type) {
  std::shared_ptr<ir::Value> value = ParseValue(expected_type);
  ir::block_num_t origin = ParseBlockValue();

  return std::make_shared<ir::InheritedValue>(value, origin);
}

// Values ::= Value (',' Value)*
std::vector<std::shared_ptr<ir::Value>> FuncParser::ParseValues(const ir::Type* expected_type) {
  std::vector<std::shared_ptr<ir::Value>> values{ParseValue(expected_type)};
  while (scanner().token() == Scanner::kComma) {
    scanner().ConsumeToken(Scanner::kComma);
    values.push_back(ParseValue(expected_type));
  }
  return values;
}

// Value ::= (Constant | Computed)
std::shared_ptr<ir::Value> FuncParser::ParseValue(const ir::Type* expected_type) {
  if (scanner().token() == Scanner::kPercentSign) {
    return ParseComputedValue(expected_type);
  } else {
    return constant_parser()->ParseConstant(expected_type);
  }
}

// Computeds ::= Computed (',' Computed)*
std::vector<std::shared_ptr<ir::Computed>> FuncParser::ParseComputedValues(
    const ir::Type* expected_type) {
  std::vector<std::shared_ptr<ir::Computed>> computeds{ParseComputedValue(expected_type)};
  while (scanner().token() == Scanner::kComma) {
    scanner().ConsumeToken(Scanner::kComma);
    computeds.push_back(ParseComputedValue(expected_type));
  }
  return computeds;
}

// Computed ::= '%' Identifier (':' Type)?
std::shared_ptr<ir::Computed> FuncParser::ParseComputedValue(const ir::Type* expected_type) {
  scanner().ConsumeToken(Scanner::kPercentSign);
  ir::value_num_t number = scanner().ConsumeInt64().value_or(ir::kNoValueNum);
  bool known = computed_values_.contains(number);

  const ir::Type* type;
  if (scanner().token() == Scanner::kColon) {
    scanner().ConsumeToken(Scanner::kColon);
    pos_t type_pos = scanner().token_start();
    type = type_parser()->ParseType();

    if (type == nullptr) {
      type = expected_type;
    } else if (expected_type != nullptr && expected_type != type) {
      issue_tracker().Add(
          ir_issues::IssueKind::kUnexpectedType, type_pos,
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
  return computed;
}

// BlockValue ::= '{' Number '}'
ir::block_num_t FuncParser::ParseBlockValue() {
  scanner().ConsumeToken(Scanner::kCurlyBracketOpen);
  ir::block_num_t number = scanner().ConsumeInt64().value_or(ir::kNoBlockNum);
  scanner().ConsumeToken(Scanner::kCurlyBracketClose);
  return number;
}

}  // namespace ir_serialization

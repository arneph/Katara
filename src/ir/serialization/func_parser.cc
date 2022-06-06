//
//  parser.cc
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "func_parser.h"

#include "src/common/logging/logging.h"

namespace ir_serialization {

// Func ::= '@' Number Identifier? FuncArgs '=>' FuncResultTypes FuncBody
ir::Func* FuncParser::ParseFunc() {
  scanner().ConsumeToken(Scanner::kAtSign);

  func_ = program()->AddFunc(scanner().ConsumeInt64());

  if (scanner().token() == Scanner::kIdentifier) {
    func_->set_name(scanner().ConsumeIdentifier());
  }

  ParseFuncArgs();
  scanner().ConsumeToken(Scanner::kArrow);
  ParseFuncResultTypes();
  ParseFuncBody();
  return func_;
}

// FuncArgs ::= '(' (Computed (',' Computed)*)? ')'
void FuncParser::ParseFuncArgs() {
  scanner().ConsumeToken(Scanner::kParenOpen);
  if (scanner().token() != Scanner::kParenClose) {
    func_->args() = ParseComputeds(/*expected_type=*/nullptr);
  }
  scanner().ConsumeToken(Scanner::kParenClose);
}

// FuncResultTypes ::= '(' (Type (',' Type)*)? ')'
void FuncParser::ParseFuncResultTypes() {
  scanner().ConsumeToken(Scanner::kParenOpen);
  if (scanner().token() != Scanner::kParenClose) {
    func_->result_types() = ParseTypes();
  }
  scanner().ConsumeToken(Scanner::kParenClose);
}

// FuncBody ::= '{' NL (NL | Block)* '}' NL
void FuncParser::ParseFuncBody() {
  scanner().ConsumeToken(Scanner::kCurlyBracketOpen);
  scanner().ConsumeToken(Scanner::kNewLine);

  for (;;) {
    if (scanner().token() == Scanner::kCurlyBracketClose) {
      scanner().ConsumeToken(Scanner::kCurlyBracketClose);
      break;
    } else if (scanner().token() == Scanner::kNewLine) {
      scanner().ConsumeToken(Scanner::kNewLine);
    } else {
      ParseBlock();
    }
  }

  scanner().ConsumeToken(Scanner::kNewLine);
  ConnectBlocks();
}

void FuncParser::ConnectBlocks() {
  for (auto& block : func_->blocks()) {
    ir::Instr* last_instr = block->instrs().back().get();

    if (last_instr->instr_kind() == ir::InstrKind::kJump) {
      ir::JumpInstr* jump = static_cast<ir::JumpInstr*>(last_instr);
      ir::block_num_t child_num = jump->destination();

      func_->AddControlFlow(block->number(), child_num);

    } else if (last_instr->instr_kind() == ir::InstrKind::kJumpCond) {
      ir::JumpCondInstr* jump_cond = static_cast<ir::JumpCondInstr*>(last_instr);
      ir::block_num_t child_a_num = jump_cond->destination_true();
      ir::block_num_t child_b_num = jump_cond->destination_false();

      func_->AddControlFlow(block->number(), child_a_num);
      func_->AddControlFlow(block->number(), child_b_num);
    }
  }
}

// Block ::= '{' Number '}' Identifier? NL Instr*
void FuncParser::ParseBlock() {
  scanner().ConsumeToken(Scanner::kCurlyBracketOpen);

  int64_t bnum = scanner().ConsumeInt64();

  scanner().ConsumeToken(Scanner::kCurlyBracketClose);

  ir::Block* block = func_->AddBlock(bnum);
  if (func_->entry_block() == nullptr) {
    func_->set_entry_block_num(bnum);
  }
  if (scanner().token() == Scanner::kIdentifier) {
    block->set_name(scanner().ConsumeIdentifier());
  }

  scanner().ConsumeToken(Scanner::kNewLine);

  for (;;) {
    if (scanner().token() == Scanner::kCurlyBracketOpen ||
        scanner().token() == Scanner::kCurlyBracketClose) {
      break;
    } else {
      block->instrs().push_back(ParseInstr());
    }
  }
}

// Instr ::= InstrResults '=' Idenifier (Value (',' Value)*)? NL
std::unique_ptr<ir::Instr> FuncParser::ParseInstr() {
  std::vector<std::shared_ptr<ir::Computed>> results = ParseInstrResults();
  std::string instr_name = scanner().ConsumeIdentifier();
  return ParseInstrWithResults(results, instr_name);
}

// InstrWithResults ::= (Value (',' Value)*)? NL
std::unique_ptr<ir::Instr> FuncParser::ParseInstrWithResults(
    std::vector<std::shared_ptr<ir::Computed>> results, std::string instr_name) {
  if (instr_name == "mov") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() + ": expected one result for mov instruction");
    }
    return ParseMovInstr(results.front());

  } else if (instr_name == "phi") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() + ": expected one result for phi instruction");
    }
    return ParsePhiInstr(results.front());

  } else if (instr_name == "conv") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() + ": expected one result for conv instruction");
    }
    return ParseConversionInstr(results.front());

  } else if (instr_name == "bnot") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() + ": expected one result for bool not instruction");
    }
    return ParseBoolNotInstr(results.front());

  } else if (auto bool_binary_op = common::ToBoolBinaryOp(instr_name); bool_binary_op) {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() +
                   ": expected one result for bool binary instruction");
    }
    return ParseBoolBinaryInstr(results.front(), bool_binary_op.value());

  } else if (auto int_unary_op = common::ToIntUnaryOp(instr_name); int_unary_op) {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() + ": expected one result for int unary instruction");
    }
    return ParseIntUnaryInstr(results.front(), int_unary_op.value());

  } else if (auto int_compare_op = common::ToIntCompareOp(instr_name); int_compare_op) {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() +
                   ": expected one result for int compare instruction");
    }
    return ParseIntCompareInstr(results.front(), int_compare_op.value());

  } else if (auto int_binary_op = common::ToIntBinaryOp(instr_name); int_binary_op) {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() + ": expected one result for int binary instruction");
    }
    return ParseIntBinaryInstr(results.front(), int_binary_op.value());

  } else if (auto int_shift_op = common::ToIntShiftOp(instr_name); int_shift_op) {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() + ": expected one result for int shift instruction");
    }
    return ParseIntShiftInstr(results.front(), int_shift_op.value());

  } else if (instr_name == "poff") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() +
                   ": expected one result for pointer offset instruction");
    }
    return ParsePointerOffsetInstr(results.front());

  } else if (instr_name == "niltest") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() + ": expected one result for nil test instruction");
    }
    return ParseNilTestInstr(results.front());

  } else if (instr_name == "malloc") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() + ": expected one result for malloc instruction");
    }
    return ParseMallocInstr(results.front());

  } else if (instr_name == "load") {
    if (results.size() != 1) {
      common::fail(scanner().PositionString() + ": expected one result for load instruction");
    }
    return ParseLoadInstr(results.front());

  } else if (instr_name == "store") {
    if (results.size() != 0) {
      common::fail(scanner().PositionString() + ": did not expect results for store instruction");
    }
    return ParseStoreInstr();

  } else if (instr_name == "free") {
    if (results.size() != 0) {
      common::fail(scanner().PositionString() + ": did not expect results for free instruction");
    }
    return ParseFreeInstr();

  } else if (instr_name == "jmp") {
    if (results.size() != 0) {
      common::fail(scanner().PositionString() + ": did not expect results for jump instruction");
    }
    return ParseJumpInstr();

  } else if (instr_name == "jcc") {
    if (results.size() != 0) {
      common::fail(scanner().PositionString() +
                   ": did not expect results for jump conditional  instruction");
    }
    return ParseJumpCondInstr();

  } else if (instr_name == "call") {
    return ParseCallInstr(results);

  } else if (instr_name == "ret") {
    if (results.size() != 0) {
      common::fail(scanner().PositionString() + ": did not expect results for return instruction");
    }
    return ParseReturnInstr();

  } else {
    common::fail(scanner().PositionString() + ": unknown operation: " + instr_name);
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
      scanner().FailForUnexpectedToken({Scanner::kNewLine, Scanner::kComma});
    }
  }

  if (args.size() < 2) {
    common::fail(scanner().PositionString() +
                 ": expected at least two arguments for phi instruction");
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
    std::shared_ptr<ir::Computed> result, common::Bool::BinaryOp op) {
  std::shared_ptr<ir::Value> operand_a = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kComma);

  std::shared_ptr<ir::Value> operand_b = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::BoolBinaryInstr>(result, op, operand_a, operand_b);
}

// IntUnaryInstr ::= Computed '=' UnaryOp Value NL
std::unique_ptr<ir::IntUnaryInstr> FuncParser::ParseIntUnaryInstr(
    std::shared_ptr<ir::Computed> result, common::Int::UnaryOp op) {
  std::shared_ptr<ir::Value> operand = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::IntUnaryInstr>(result, op, operand);
}

// IntCompareInstr ::= Computed '=' CompareOp Value ',' Value NL
std::unique_ptr<ir::IntCompareInstr> FuncParser::ParseIntCompareInstr(
    std::shared_ptr<ir::Computed> result, common::Int::CompareOp op) {
  std::shared_ptr<ir::Value> operand_a = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kComma);

  std::shared_ptr<ir::Value> operand_b = ParseValue(operand_a->type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::IntCompareInstr>(result, op, operand_a, operand_b);
}

// IntBinaryInstr ::= Computed '=' BinaryOp Value ',' Value NL
std::unique_ptr<ir::IntBinaryInstr> FuncParser::ParseIntBinaryInstr(
    std::shared_ptr<ir::Computed> result, common::Int::BinaryOp op) {
  std::shared_ptr<ir::Value> operand_a = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kComma);

  std::shared_ptr<ir::Value> operand_b = ParseValue(result->type());
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::IntBinaryInstr>(result, op, operand_a, operand_b);
}

// IntShiftInstr ::= Computed '=' ShiftOp Value ',' Value NL
std::unique_ptr<ir::IntShiftInstr> FuncParser::ParseIntShiftInstr(
    std::shared_ptr<ir::Computed> result, common::Int::ShiftOp op) {
  std::shared_ptr<ir::Value> operand_a = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kComma);

  std::shared_ptr<ir::Value> operand_b = ParseValue(/*expected_type=*/nullptr);
  scanner().ConsumeToken(Scanner::kNewLine);

  return std::make_unique<ir::IntShiftInstr>(result, op, operand_a, operand_b);
}

// PointerOffsetInstr ::= Computed '=' 'poff' Value ',' Value NL
std::unique_ptr<ir::PointerOffsetInstr> FuncParser::ParsePointerOffsetInstr(
    std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Computed> pointer = ParseComputed(ir::pointer_type());
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
    results = ParseComputeds(/*expected_type=*/nullptr);
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
    return ParseComputed(expected_type);
  } else {
    return ParseConstant(expected_type);
  }
}

std::shared_ptr<ir::Constant> FuncParser::ParseConstant(const ir::Type* expected_type) {
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
std::shared_ptr<ir::PointerConstant> FuncParser::ParsePointerConstant() {
  uint64_t number = scanner().token_address().AsUint64();
  scanner().Next();

  return ir::ToPointerConstant(number);
}

// FuncConstant ::= '@' Number
std::shared_ptr<ir::FuncConstant> FuncParser::ParseFuncConstant() {
  scanner().ConsumeToken(Scanner::kAtSign);
  ir::func_num_t number = scanner().ConsumeInt64();

  return ir::ToFuncConstant(number);
}

// Constant ::= '#t' | '#f' | '#' Number (':' Type)?
std::shared_ptr<ir::Constant> FuncParser::ParseBoolOrIntConstant(const ir::Type* expected_type) {
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
    auto type = ParseType();
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

// Computeds ::= Computed (',' Computed)*
std::vector<std::shared_ptr<ir::Computed>> FuncParser::ParseComputeds(
    const ir::Type* expected_type) {
  std::vector<std::shared_ptr<ir::Computed>> computeds{ParseComputed(expected_type)};
  while (scanner().token() == Scanner::kComma) {
    scanner().ConsumeToken(Scanner::kComma);
    computeds.push_back(ParseComputed(expected_type));
  }
  return computeds;
}

// Computed ::= '%' Identifier (':' Type)?
std::shared_ptr<ir::Computed> FuncParser::ParseComputed(const ir::Type* expected_type) {
  scanner().ConsumeToken(Scanner::kPercentSign);
  ir::value_num_t number = scanner().ConsumeInt64();
  bool known = computed_values_.contains(number);

  const ir::Type* type;
  if (scanner().token() == Scanner::kColon) {
    scanner().ConsumeToken(Scanner::kColon);
    type = ParseType();

    if (expected_type != nullptr && expected_type != type) {
      common::fail(scanner().PositionString() + ": expected '" + expected_type->RefString() +
                   "'; got '" + type->RefString() + "'");
    }
  } else {
    if (expected_type == nullptr && !known) {
      scanner().FailForUnexpectedToken({Scanner::kColon});
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
  ir::block_num_t number = scanner().ConsumeInt64();
  scanner().ConsumeToken(Scanner::kCurlyBracketClose);
  return number;
}

// Types ::= Type (',' Type)?
std::vector<const ir::Type*> FuncParser::ParseTypes() {
  std::vector<const ir::Type*> types{ParseType()};
  while (scanner().token() == Scanner::kComma) {
    scanner().ConsumeToken(Scanner::kComma);
    types.push_back(ParseType());
  }
  return types;
}

// Type ::= Identifier
const ir::Type* FuncParser::ParseType() {
  std::string name = scanner().ConsumeIdentifier();

  if (name == "b") {
    return ir::bool_type();
  } else if (auto int_type = common::ToIntType(name); int_type) {
    return ir::IntTypeFor(int_type.value());
  } else if (name == "ptr") {
    return ir::pointer_type();
  } else if (name == "func") {
    return ir::func_type();
  } else {
    common::fail(scanner().PositionString() + ": unexpected type");
  }
}

}  // namespace ir_serialization

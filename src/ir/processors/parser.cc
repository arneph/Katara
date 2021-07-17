//
//  parser.cc
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "parser.h"

namespace ir_proc {

std::unique_ptr<ir::Program> Parser::Parse(std::istream& in_stream) {
  Scanner scanner(in_stream);
  return Parse(scanner);
}

std::unique_ptr<ir::Program> Parser::Parse(Scanner& scanner) {
  Parser parser(scanner);

  parser.ParseProgram();

  return std::move(parser.program_);
}

Parser::Parser(Scanner& scanner) : scanner_(scanner) {
  scanner_.Next();
  program_ = std::make_unique<ir::Program>();
}

// Program ::= (Func | NL)*
void Parser::ParseProgram() {
  for (;;) {
    if (scanner_.token() == Scanner::kNewLine) {
      scanner_.Next();
    } else if (scanner_.token() == Scanner::kAtSign) {
      ParseFunc();
    } else if (scanner_.token() == Scanner::kEoF) {
      break;
    } else {
      throw "unexpected token";
    }
  }
}

// Func ::= '@' Number Identifier? FuncArgs '=>' FuncResultTypes FuncBody
void Parser::ParseFunc() {
  if (scanner_.token() != Scanner::kAtSign) throw "expected '@'";
  scanner_.Next();

  if (scanner_.token() != Scanner::kNumber) throw "expected number";

  ir::Func* func = program_->AddFunc(scanner_.number());

  scanner_.Next();

  if (scanner_.token() == Scanner::kIdentifier) {
    func->set_name(scanner_.string());

    scanner_.Next();
  }

  ParseFuncArgs(func);

  if (scanner_.token() != Scanner::kArrow) throw "expected '=>'";
  scanner_.Next();

  ParseFuncResultTypes(func);
  ParseFuncBody(func);
}

// FuncArgs ::= '(' (Computed (',' Computed)*)? ')'
void Parser::ParseFuncArgs(ir::Func* func) {
  if (scanner_.token() != Scanner::kRoundBracketOpen) throw "expected '('";
  scanner_.Next();

  if (scanner_.token() == Scanner::kRoundBracketClose) {
    scanner_.Next();
  } else {
    for (;;) {
      std::shared_ptr<ir::Computed> arg = ParseComputed(/*expected_type=*/nullptr);

      func->args().push_back(arg);

      if (scanner_.token() == Scanner::kRoundBracketClose) {
        scanner_.Next();
        break;
      } else if (scanner_.token() == Scanner::kComma) {
        scanner_.Next();
      } else {
        throw "expected ')' or ','";
      }
    }
  }
}

// FuncResultTypes ::= '(' (Type (',' Type)*)? ')'
void Parser::ParseFuncResultTypes(ir::Func* func) {
  if (scanner_.token() != Scanner::kRoundBracketOpen) throw "expected '('";
  scanner_.Next();

  if (scanner_.token() == Scanner::kRoundBracketClose) {
    scanner_.Next();
  } else {
    for (;;) {
      const ir::AtomicType* type = ParseType();

      func->result_types().push_back(type);

      if (scanner_.token() == Scanner::kRoundBracketClose) {
        scanner_.Next();
        break;
      } else if (scanner_.token() == Scanner::kComma) {
        scanner_.Next();
      } else {
        throw "expected ')' or ','";
      }
    }
  }
}

// FuncBody ::= '{' NL (NL | Block)* '}' NL
void Parser::ParseFuncBody(ir::Func* func) {
  if (scanner_.token() != Scanner::kCurlyBracketOpen) throw "expected '{'";
  scanner_.Next();
  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  for (;;) {
    if (scanner_.token() == Scanner::kCurlyBracketClose) {
      scanner_.Next();
      break;
    } else if (scanner_.token() == Scanner::kNewLine) {
      scanner_.Next();
    } else {
      ParseBlock(func);
    }
  }

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  ConnectBlocks(func);
}

void Parser::ConnectBlocks(ir::Func* func) {
  for (auto& block : func->blocks()) {
    ir::Instr* last_instr = block->instrs().back().get();

    if (ir::JumpInstr* jump = dynamic_cast<ir::JumpInstr*>(last_instr)) {
      ir::block_num_t child_num = jump->destination();

      func->AddControlFlow(block->number(), child_num);

    } else if (ir::JumpCondInstr* jump_cond = dynamic_cast<ir::JumpCondInstr*>(last_instr)) {
      ir::block_num_t child_a_num = jump_cond->destination_true();
      ir::block_num_t child_b_num = jump_cond->destination_false();

      func->AddControlFlow(block->number(), child_a_num);
      func->AddControlFlow(block->number(), child_b_num);
    }
  }
}

// Block ::= '{' Number '}' Identifier? NL Instr*
void Parser::ParseBlock(ir::Func* func) {
  if (scanner_.token() != Scanner::kCurlyBracketOpen) throw "expected '{‘";
  scanner_.Next();

  if (scanner_.token() != Scanner::kNumber) throw "expected number";
  int64_t bnum = scanner_.number();
  scanner_.Next();

  if (scanner_.token() != Scanner::kCurlyBracketClose) throw "expected '}'";
  scanner_.Next();

  ir::Block* block = func->AddBlock(bnum);

  if (func->entry_block() == nullptr) {
    func->set_entry_block_num(bnum);
  }

  if (scanner_.token() == Scanner::kIdentifier) {
    block->set_name(scanner_.string());

    scanner_.Next();
  }

  if (scanner_.token() != Scanner::kNewLine) throw "expected identifier or new line";
  scanner_.Next();

  for (;;) {
    if (scanner_.token() == Scanner::kCurlyBracketOpen ||
        scanner_.token() == Scanner::kCurlyBracketClose) {
      break;
    } else {
      block->instrs().push_back(ParseInstr());
    }
  }
}

// Instr ::= InstrResults Idenifier (':' Type)? (Value (',' Value)*)? NL
std::unique_ptr<ir::Instr> Parser::ParseInstr() {
  std::vector<std::shared_ptr<ir::Computed>> results = ParseInstrResults();

  if (scanner_.token() != Scanner::kIdentifier) {
    throw "expected '%' or identifier";
  }
  std::string instr_name = scanner_.string();
  scanner_.Next();

  if (instr_name == "mov") {
    if (results.size() != 1) {
      throw "expected one result for mov instruction";
    }
    return ParseMovInstr(results.front());

  } else if (instr_name == "phi") {
    if (results.size() != 1) {
      throw "expected one result for phi instruction";
    }
    return ParsePhiInstr(results.front());

  } else if (instr_name == "conv") {
    if (results.size() != 1) {
      throw "expected one result for conv instruction";
    }
    return ParseConversionInstr(results.front());

  } else if (instr_name == "bnot") {
    if (results.size() != 1) {
      throw "expected one result for bool not instruction";
    }
    return ParseBoolNotInstr(results.front());

  } else if (auto bool_binary_op = common::ToBoolBinaryOp(instr_name); bool_binary_op) {
    if (results.size() != 1) {
      throw "expected one result for bool binary instruction";
    }
    return ParseBoolBinaryInstr(results.front(), bool_binary_op.value());

  } else if (auto int_unary_op = common::ToIntUnaryOp(instr_name); int_unary_op) {
    if (results.size() != 1) {
      throw "expected one result for int unary instruction";
    }
    return ParseIntUnaryInstr(results.front(), int_unary_op.value());

  } else if (auto int_compare_op = common::ToIntCompareOp(instr_name); int_compare_op) {
    if (results.size() != 1) {
      throw "expected one result for int compare instruction";
    }
    return ParseIntCompareInstr(results.front(), int_compare_op.value());

  } else if (auto int_binary_op = common::ToIntBinaryOp(instr_name); int_binary_op) {
    if (results.size() != 1) {
      throw "expected one result for int binary instruction";
    }
    return ParseIntBinaryInstr(results.front(), int_binary_op.value());

  } else if (auto int_shift_op = common::ToIntShiftOp(instr_name); int_shift_op) {
    if (results.size() != 1) {
      throw "expected one result for int shift instruction";
    }
    return ParseIntShiftInstr(results.front(), int_shift_op.value());

  } else if (instr_name == "jmp") {
    if (results.size() != 0) throw "did not expect results for jump instruction";

    return ParseJumpInstr();

  } else if (instr_name == "jcc") {
    if (results.size() != 0) throw "did not expect results for jump conditional  instruction";

    return ParseJumpCondInstr();

  } else if (instr_name == "call") {
    return ParseCallInstr(results);

  } else if (instr_name == "ret") {
    if (results.size() != 0) throw "did not expect results for return instruction";

    return ParseReturnInstr();

  } else {
    throw "unknown operation: " + instr_name;
  }
}

// MovInstr ::= Computed 'mov' Value NL
std::unique_ptr<ir::MovInstr> Parser::ParseMovInstr(std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> arg = ParseValue(result->type());

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::MovInstr>(result, arg);
}

// PhiInstr ::= Computed 'phi' InheritedValue (',' InheritedValue)+ NL
std::unique_ptr<ir::PhiInstr> Parser::ParsePhiInstr(std::shared_ptr<ir::Computed> result) {
  std::vector<std::shared_ptr<ir::InheritedValue>> args;

  args.push_back(ParseInheritedValue(result->type()));

  for (;;) {
    if (scanner_.token() == Scanner::kNewLine) {
      scanner_.Next();
      break;
    } else if (scanner_.token() == Scanner::kComma) {
      scanner_.Next();

      args.push_back(ParseInheritedValue(result->type()));

    } else {
      throw "expected ',' or new line";
    }
  }

  if (args.size() < 2) throw "expected at least two arguments for phi instruction";

  return std::make_unique<ir::PhiInstr>(result, args);
}

// MovInstr ::= Computed 'conv' Value NL
std::unique_ptr<ir::Conversion> Parser::ParseConversionInstr(std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> arg = ParseValue(/*expected_type=*/nullptr);

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::Conversion>(result, arg);
}

// BoolNotInstr ::= Computed 'bnot' Value NL
std::unique_ptr<ir::BoolNotInstr> Parser::ParseBoolNotInstr(std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> operand = ParseValue(&ir::kBool);

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::BoolNotInstr>(result, operand);
}

// BoolBinaryInstr ::= Computed BinaryOp Value NL
std::unique_ptr<ir::BoolBinaryInstr> Parser::ParseBoolBinaryInstr(
    std::shared_ptr<ir::Computed> result, common::Bool::BinaryOp op) {
  std::shared_ptr<ir::Value> operand_a = ParseValue(result->type());

  if (scanner_.token() != Scanner::kComma) throw "expected ','";
  scanner_.Next();

  std::shared_ptr<ir::Value> operand_b = ParseValue(result->type());

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::BoolBinaryInstr>(result, op, operand_a, operand_b);
}

// IntUnaryInstr ::= Computed UnaryOp Value NL
std::unique_ptr<ir::IntUnaryInstr> Parser::ParseIntUnaryInstr(std::shared_ptr<ir::Computed> result,
                                                              common::Int::UnaryOp op) {
  std::shared_ptr<ir::Value> operand = ParseValue(result->type());

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::IntUnaryInstr>(result, op, operand);
}

// IntCompareInstr ::= Computed CompareOp Value NL
std::unique_ptr<ir::IntCompareInstr> Parser::ParseIntCompareInstr(
    std::shared_ptr<ir::Computed> result, common::Int::CompareOp op) {
  std::shared_ptr<ir::Value> operand_a = ParseValue(/*expected_type=*/nullptr);

  if (scanner_.token() != Scanner::kComma) throw "expected ','";
  scanner_.Next();

  std::shared_ptr<ir::Value> operand_b = ParseValue(operand_a->type());

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::IntCompareInstr>(result, op, operand_a, operand_b);
}

// IntBinaryInstr ::= Computed BinaryOp Value NL
std::unique_ptr<ir::IntBinaryInstr> Parser::ParseIntBinaryInstr(
    std::shared_ptr<ir::Computed> result, common::Int::BinaryOp op) {
  std::shared_ptr<ir::Value> operand_a = ParseValue(result->type());

  if (scanner_.token() != Scanner::kComma) throw "expected ','";
  scanner_.Next();

  std::shared_ptr<ir::Value> operand_b = ParseValue(result->type());

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::IntBinaryInstr>(result, op, operand_a, operand_b);
}

// IntShiftInstr ::= Computed ShiftOp Value NL
std::unique_ptr<ir::IntShiftInstr> Parser::ParseIntShiftInstr(std::shared_ptr<ir::Computed> result,
                                                              common::Int::ShiftOp op) {
  std::shared_ptr<ir::Value> operand_a = ParseValue(/*expected_type=*/nullptr);

  if (scanner_.token() != Scanner::kComma) throw "expected ','";
  scanner_.Next();

  std::shared_ptr<ir::Value> operand_b = ParseValue(/*expected_type=*/nullptr);

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::IntShiftInstr>(result, op, operand_a, operand_b);
}

// JumpInstr ::= 'jmp' BlockValue NL
std::unique_ptr<ir::JumpInstr> Parser::ParseJumpInstr() {
  ir::block_num_t destination = ParseBlockValue();

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::JumpInstr>(destination);
}

// JumpCondInstr ::= 'jcc' Value ',' BlockValue ',' BlockValue NL
std::unique_ptr<ir::JumpCondInstr> Parser::ParseJumpCondInstr() {
  std::shared_ptr<ir::Value> condition = ParseValue(&ir::kBool);

  if (scanner_.token() != Scanner::kComma) throw "expected ','";
  scanner_.Next();

  ir::block_num_t destination_true = ParseBlockValue();

  if (scanner_.token() != Scanner::kComma) throw "expected ','";
  scanner_.Next();

  ir::block_num_t destination_false = ParseBlockValue();

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::JumpCondInstr>(condition, destination_true, destination_false);
}

// CallInstr ::= (Computed (',' Computed)* '=')?
//               'call' Value (',' Value)* NL
std::unique_ptr<ir::CallInstr> Parser::ParseCallInstr(
    std::vector<std::shared_ptr<ir::Computed>> results) {
  std::shared_ptr<ir::Value> func = ParseValue(&ir::kFunc);

  std::vector<std::shared_ptr<ir::Value>> args;

  for (;;) {
    if (scanner_.token() == Scanner::kNewLine) {
      scanner_.Next();
      break;
    } else if (scanner_.token() == Scanner::kComma) {
      scanner_.Next();

      args.push_back(ParseValue(/*expected_type=*/nullptr));

    } else {
      throw "expected ',' or NL";
    }
  }

  return std::make_unique<ir::CallInstr>(func, results, args);
}

std::unique_ptr<ir::ReturnInstr> Parser::ParseReturnInstr() {
  std::vector<std::shared_ptr<ir::Value>> args;

  if (scanner_.token() == Scanner::kNewLine) {
    scanner_.Next();

    return std::make_unique<ir::ReturnInstr>(args);
  }

  args.push_back(ParseValue(/*expected_type=*/nullptr));

  for (;;) {
    if (scanner_.token() == Scanner::kNewLine) {
      scanner_.Next();
      break;

    } else if (scanner_.token() == Scanner::kComma) {
      scanner_.Next();

      args.push_back(ParseValue(/*expected_type=*/nullptr));

    } else {
      throw "expected ',' or new line";
    }
  }

  return std::make_unique<ir::ReturnInstr>(args);
}

// InstrResults ::= (Computed (',' Computed)* '=')?
std::vector<std::shared_ptr<ir::Computed>> Parser::ParseInstrResults() {
  std::vector<std::shared_ptr<ir::Computed>> results;

  if (scanner_.token() == Scanner::kPercentSign) {
    for (;;) {
      results.push_back(ParseComputed(/*expected_type=*/nullptr));

      if (scanner_.token() == Scanner::kEqualSign) {
        scanner_.Next();
        break;
      } else if (scanner_.token() == Scanner::kComma) {
        scanner_.Next();
      } else {
        throw "expected ',' or '='";
      }
    }
  }

  return results;
}

// InheritedValue ::= (Constant | Computed) BlockValue
std::shared_ptr<ir::InheritedValue> Parser::ParseInheritedValue(const ir::Type* expected_type) {
  std::shared_ptr<ir::Value> value = ParseValue(expected_type);
  ir::block_num_t origin = ParseBlockValue();

  return std::make_shared<ir::InheritedValue>(value, origin);
}

// Value ::= (Constant | Computed | BlockValue)
std::shared_ptr<ir::Value> Parser::ParseValue(const ir::Type* expected_type) {
  switch (scanner_.token()) {
    case Scanner::kAtSign:
    case Scanner::kHashSign:
    case Scanner::kCurlyBracketOpen:
      return ParseConstant(expected_type);
    case Scanner::kPercentSign:
      return ParseComputed(expected_type);
    default:
      throw "expected '#', '%', '{', or '@'";
  }
}

// Constant ::= '@' Number
//              | '#t' | '#f'
//              | '#' Number (':' Type)?
std::shared_ptr<ir::Constant> Parser::ParseConstant(const ir::Type* expected_type) {
  if (scanner_.token() == Scanner::kAtSign) {
    if (expected_type != &ir::kFunc) throw "unexpected '@'";
    scanner_.Next();

    if (scanner_.token() != Scanner::kNumber) throw "expected number";
    ir::func_num_t number = scanner_.number();
    scanner_.Next();

    return std::make_shared<ir::FuncConstant>(number);
  }

  if (scanner_.token() != Scanner::kHashSign) throw "expected '@' or '#'";
  scanner_.Next();

  if (scanner_.token() == Scanner::kIdentifier) {
    if (scanner_.string() == "f") {
      if (expected_type != &ir::kBool) throw "unexpected 'f'";

      return std::make_shared<ir::BoolConstant>(false);
    } else if (scanner_.string() == "t") {
      if (expected_type != &ir::kBool) throw "unexpected 't'";

      return std::make_shared<ir::BoolConstant>(true);
    } else {
      throw "expected number, 't' or 'f'";
    }
  }

  if (scanner_.token() != Scanner::kNumber) throw "expected number, 't' or 'f'";
  int64_t sign = scanner_.sign();
  uint64_t number = scanner_.number();
  scanner_.Next();

  common::IntType int_type;
  if (scanner_.token() == Scanner::kColon) {
    scanner_.Next();
    auto type = ParseType();
    if (type->type_kind() != ir::TypeKind::kInt) {
      throw "expected int type";
    } else if (expected_type != nullptr && expected_type != type) {
      throw "expected: " + expected_type->ToString() + " got: " + type->ToString();
    }
    int_type = static_cast<const ir::IntType*>(type)->int_type();

  } else {
    if (expected_type == nullptr) {
      throw "expected ':'";
    } else if (expected_type->type_kind() != ir::TypeKind::kInt) {
      throw "expected: " + expected_type->ToString();
    }

    int_type = static_cast<const ir::IntType*>(expected_type)->int_type();
  }
  common::Int value(number);
  value = value.ConvertTo(int_type);
  if (sign == -1) {
    value = common::Int::Compute(common::Int::UnaryOp::kNeg, value);
  }

  return std::make_shared<ir::IntConstant>(value);
}

// Computed ::= '%' Identifier (':' Type)?
std::shared_ptr<ir::Computed> Parser::ParseComputed(const ir::Type* expected_type) {
  if (scanner_.token() != Scanner::kPercentSign) throw "expected '%'";
  scanner_.Next();

  if (scanner_.token() != Scanner::kNumber) throw "expected number";
  int64_t number = scanner_.number();
  scanner_.Next();

  const ir::Type* type;
  if (scanner_.token() == Scanner::kColon) {
    scanner_.Next();
    type = ParseType();

    if (expected_type != nullptr && expected_type != type)
      throw "expected: " + expected_type->ToString() + " got: " + type->ToString();
  } else {
    if (expected_type == nullptr) throw "expected ':'";

    type = expected_type;
  }

  return std::make_shared<ir::Computed>(type, number);
}

ir::block_num_t Parser::ParseBlockValue() {
  if (scanner_.token() != Scanner::kCurlyBracketOpen) throw "expected '{'";
  scanner_.Next();

  if (scanner_.token() != Scanner::kNumber) {
    throw "expected number";
  }
  ir::block_num_t number = scanner_.number();
  scanner_.Next();

  if (scanner_.token() != Scanner::kCurlyBracketClose) {
    throw "expected '}'";
  }
  scanner_.Next();

  return number;
}

// Type ::= Identifier
const ir::AtomicType* Parser::ParseType() {
  if (scanner_.token() != Scanner::kIdentifier) throw "expected identifier";
  std::string name = scanner_.string();
  scanner_.Next();

  if (name == "b") {
    return &ir::kBool;
  } else if (auto int_type = common::ToIntType(name); int_type) {
    return ir::IntTypeFor(int_type.value());
  } else if (name == "ptr") {
    return &ir::kPointer;
  } else if (name == "func") {
    return &ir::kFunc;
  } else {
    throw "unexpected type";
  }
}

}  // namespace ir_proc

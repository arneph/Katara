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

  parser.ParseProg();

  return std::move(parser.program_);
}

Parser::Parser(Scanner& scanner) : scanner_(scanner) {
  scanner_.Next();
  program_ = std::make_unique<ir::Program>();
}

// Prog ::= (Func | NL)*
void Parser::ParseProg() {
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
      std::shared_ptr<ir::Computed> arg = ParseComputed(/*expected_type=*/std::nullopt);

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
      ir::AtomicType* type = ParseType();

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

  if (scanner_.token() != Scanner::kIdentifier) throw "expected '%' or identifier";
  std::string instr_name = scanner_.string();
  scanner_.Next();

  if (instr_name == "mov") {
    if (results.size() != 1) throw "expected one result for mov instruction";

    return ParseMovInstr(results.at(0));

  } else if (instr_name == "phi") {
    if (results.size() != 1) throw "expected one result for phi instruction";

    return ParsePhiInstr(results.at(0));

  } else if (ir::IsUnaryALOperationString(instr_name)) {
    ir::UnaryALOperation op = ir::ToUnaryALOperation(instr_name);

    if (results.size() != 1) throw "expected one result for unary al instruction";

    return ParseUnaryALInstr(results.at(0), op);

  } else if (ir::IsBinaryALOperationString(instr_name)) {
    ir::BinaryALOperation op = ir::ToBinaryALOperation(instr_name);

    if (results.size() != 1) throw "expected one result for binary al instruction";

    return ParseBinaryALInstr(results.at(0), op);

  } else if (ir::IsCompareOperationString(instr_name)) {
    ir::CompareOperation op = ir::ToCompareOperation(instr_name);

    if (results.size() != 1) throw "expected one result for compare instruction";

    return ParseCompareInstr(results.at(0), op);

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

  /*
  bool has_instr_type = false;
  ir::Type instr_type = ir::Type::kUndefined;

  if (scanner_.token() == Scanner::kColon) {
      scanner_.Next();

      has_instr_type = true;
      instr_type = ParseType();
  }

  std::vector<ir::Value> args = ParseInstrArgs(instr_type);

  if (instr_name == "mov") {
      if (!has_instr_type)
          throw "expected type for mov instruction";
      if (results.size() != 1)
          throw "expected one result for mov instruction";
      if (args.size() != 1)
          throw "expected one argument for mov instruction";

      ir::Computed result(instr_type, results.at(0).number());

      return new ir::MovInstr(, args.at(0));

  } else if (instr_name == "phi") {
      if (!has_instr_type)
          throw "expected type for phi instruction";
      if (results.size() != 1)
          throw "expected one result for phi instruction";
      if (args.size() < 2)
          throw "expected at least two args for phi instruction";

      ir::Computed result(instr_type, results.at(0).number());

      std::vector<ir::InheritedValue> inherited_args;
      inherited_args.reserve(args.size());

      for (ir::Value arg : args) {
          ir::InheritedValue *inherited_arg =
              dynamic_cast<ir::InheritedValue *>(arg);
          if (inherited_arg == nullptr)
              throw "expected only inherited value args for phi instruction";

          inherited_args.push_back(inherited_arg);
      }

      return new ir::PhiInstr(result, inherited_args);

  } else if (instr_name == "not" ||
             instr_name == "neg") {
      if (!has_instr_type)
          throw "expected type for unary al instruction";
      if (results.size() != 1)
          throw "expected one result for unary al instruction";
      if (args.size() != 1)
          throw "expected one operand for unary al instruction";

      ir::UnaryALOperation op = ir::to_unary_al_operation(instr_name);

      ir::Computed *result = ReplaceComputed(instr_type,
                                             results.at(0)->name());

      return new ir::UnaryALInstr(op, result, args.at(0));

  } else if (instr_name == "and" ||
             instr_name == "or" ||
             instr_name == "xor" ||
             instr_name == "add" ||
             instr_name == "sub" ||
             instr_name == "mul" ||
             instr_name == "div" ||
             instr_name == "rem") {
      if (!has_instr_type)
          throw "expected type for binary al instruction";
      if (results.size() != 1)
          throw "expected one result for binary al instruction";
      if (args.size() != 2)
          throw "expected two operands for binary al instruction";

      ir::BinaryALOperation op = ir::to_binary_al_operation(instr_name);

      ir::Computed *result = ReplaceComputed(instr_type,
                                             results.at(0)->name());

      return new ir::BinaryALInstr(op, result,
                                   args.at(0),
                                   args.at(1));

  } else if (instr_name == "eq" ||
             instr_name == "ne" ||
             instr_name == "gt" ||
             instr_name == "gte" ||
             instr_name == "lte" ||
             instr_name == "lt") {
      if (!has_instr_type)
          throw "expected type for compare instruction";
      if (results.size() != 1)
         throw "expected one result for compare instruction";
      if (args.size() != 2)
         throw "expected two operands for compare instruction";

      ir::CompareOperation op = ir::to_compare_operation(instr_name);

      ir::Computed *result = ReplaceComputed(ir::kBool,
                                             results.at(0)->name());

      return new ir::CompareInstr(op, result,
                                  args.at(0),
                                  args.at(1));

  } else if (instr_name == "jmp") {
      if (has_instr_type)
          throw "did not expected type for jump instruction";
      if (results.size() != 0)
          throw "did not expect results for jump instruction";
      if (args.size() != 1)
          throw "expected one operand for jump instruction";

      ir::BlockValue *destination
          = dynamic_cast<ir::BlockValue *>(args.at(0));
      if (destination == nullptr)
          throw "expected block value arg for jump instruction";

      return new ir::JumpInstr(destination);

  } else if (instr_name == "jcc") {
      if (has_instr_type)
          throw "did not expected type for jump conditional instruction";
      if (results.size() != 0)
          throw "did not expect results for jump conditional instruction";
      if (args.size() != 3)
          throw "expected three operands for jump conditional instruction";

      ir::Value *decision_value = args.at(0);
      ir::Computed *computed =
          dynamic_cast<ir::Computed *>(decision_value);
      if (computed != nullptr) {
          decision_value = ReplaceComputed(ir::kBool,
                                           computed->name());
      }

      ir::BlockValue *destination_true
          = dynamic_cast<ir::BlockValue *>(args.at(1));
      if (destination_true == nullptr)
          throw "expected block value as second arg for jump conditional instruction";

      ir::BlockValue *destination_false
          = dynamic_cast<ir::BlockValue *>(args.at(2));
      if (destination_false == nullptr)
          throw "expected block value as third arg for jump conditional instruction";

      return new ir::JumpCondInstr(decision_value,
                                   destination_true,
                                   destination_false);

  } else if (instr_name == "call") {
      if (has_instr_type)
          throw "did not expected type for call instruction";
      if (args.size() < 1)
          throw "expected at least one operand for call instruction";

      ir::Value *func = args.at(0);

      args = std::vector<ir::Value *>(args.begin() + 1,
                                      args.end());

      return new ir::CallInstr(func, results, args);

  } else if (instr_name == "ret") {
      if (has_instr_type)
          throw "did not expected type for return instruction";


      return new ir::ReturnInstr(args);

  } else {
      throw "unknown operation: " + instr_name;
  }*/
}

// MovInstr ::= Computed 'mov' Value NL
std::unique_ptr<ir::MovInstr> Parser::ParseMovInstr(std::shared_ptr<ir::Computed> result) {
  std::shared_ptr<ir::Value> arg = ParseValue(static_cast<ir::AtomicType*>(result->type())->kind());

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::MovInstr>(result, arg);
}

// PhiInstr ::= Computed 'phi' InheritedValue (',' InheritedValue)+ NL
std::unique_ptr<ir::PhiInstr> Parser::ParsePhiInstr(std::shared_ptr<ir::Computed> result) {
  std::vector<std::shared_ptr<ir::InheritedValue>> args;

  args.push_back(ParseInheritedValue(static_cast<ir::AtomicType*>(result->type())->kind()));

  for (;;) {
    if (scanner_.token() == Scanner::kNewLine) {
      scanner_.Next();
      break;
    } else if (scanner_.token() == Scanner::kComma) {
      scanner_.Next();

      args.push_back(ParseInheritedValue(static_cast<ir::AtomicType*>(result->type())->kind()));

    } else {
      throw "expected ',' or new line";
    }
  }

  if (args.size() < 2) throw "expected at least two arguments for phi instruction";

  return std::make_unique<ir::PhiInstr>(result, args);
}

// UnaryALInstr ::= Computed UnaryALOp ':' Type Value NL
std::unique_ptr<ir::UnaryALInstr> Parser::ParseUnaryALInstr(std::shared_ptr<ir::Computed> result,
                                                            ir::UnaryALOperation op) {
  if (scanner_.token() != Scanner::kColon) throw "expected ':'";
  scanner_.Next();

  ir::AtomicType* instr_type = ParseType();
  if (instr_type != result->type()) throw "result type does not match type of unary AL instruction";

  std::shared_ptr<ir::Value> operand = ParseValue(instr_type->kind());

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::UnaryALInstr>(op, result, operand);
}

// BinaryALInstr ::= Computed BinaryALOp ':' Type Value ',' Value NL
std::unique_ptr<ir::BinaryALInstr> Parser::ParseBinaryALInstr(std::shared_ptr<ir::Computed> result,
                                                              ir::BinaryALOperation op) {
  if (scanner_.token() != Scanner::kColon) throw "expected ':'";
  scanner_.Next();

  ir::AtomicType* instr_type = ParseType();
  if (instr_type != result->type())
    throw "result type does not match type of binary AL instruction";

  std::shared_ptr<ir::Value> operand_a = ParseValue(instr_type->kind());

  if (scanner_.token() != Scanner::kComma) throw "expected ','";
  scanner_.Next();

  std::shared_ptr<ir::Value> operand_b = ParseValue(instr_type->kind());

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::BinaryALInstr>(op, result, operand_a, operand_b);
}

// CompareInstr ::= Computed CompareOp ':' Type Value ',' Value NL
std::unique_ptr<ir::CompareInstr> Parser::ParseCompareInstr(std::shared_ptr<ir::Computed> result,
                                                            ir::CompareOperation op) {
  if (scanner_.token() != Scanner::kColon) throw "expected ':'";
  scanner_.Next();

  ir::AtomicType* instr_type = ParseType();
  std::shared_ptr<ir::Value> operand_a = ParseValue(instr_type->kind());

  if (scanner_.token() != Scanner::kComma) throw "expected ','";
  scanner_.Next();

  std::shared_ptr<ir::Value> operand_b = ParseValue(instr_type->kind());

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return std::make_unique<ir::CompareInstr>(op, result, operand_a, operand_b);
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
  std::shared_ptr<ir::Value> condition = ParseValue(ir::AtomicTypeKind::kBool);

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
  std::shared_ptr<ir::Value> func = ParseValue(ir::AtomicTypeKind::kFunc);

  std::vector<std::shared_ptr<ir::Value>> args;

  for (;;) {
    if (scanner_.token() == Scanner::kNewLine) {
      scanner_.Next();
      break;
    } else if (scanner_.token() == Scanner::kComma) {
      scanner_.Next();

      args.push_back(ParseValue(/*expected_type=*/std::nullopt));

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

  args.push_back(ParseValue(/*expected_type=*/std::nullopt));

  for (;;) {
    if (scanner_.token() == Scanner::kNewLine) {
      scanner_.Next();
      break;

    } else if (scanner_.token() == Scanner::kComma) {
      scanner_.Next();

      args.push_back(ParseValue(/*expected_type=*/std::nullopt));

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
      results.push_back(ParseComputed(/*expected_type=*/std::nullopt));

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
std::shared_ptr<ir::InheritedValue> Parser::ParseInheritedValue(
    std::optional<ir::AtomicTypeKind> expected_type) {
  std::shared_ptr<ir::Value> value = ParseValue(expected_type);
  ir::block_num_t origin = ParseBlockValue();

  return std::make_shared<ir::InheritedValue>(value, origin);
}

// Value ::= (Constant | Computed | BlockValue)
std::shared_ptr<ir::Value> Parser::ParseValue(std::optional<ir::AtomicTypeKind> expected_type) {
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
std::shared_ptr<ir::Constant> Parser::ParseConstant(
    std::optional<ir::AtomicTypeKind> expected_type) {
  if (scanner_.token() == Scanner::kAtSign) {
    if (expected_type != ir::AtomicTypeKind::kFunc) throw "unexpected '@'";
    scanner_.Next();

    if (scanner_.token() != Scanner::kNumber) throw "expected number";
    ir::func_num_t number = scanner_.number();
    scanner_.Next();

    ir::AtomicType* func_type =
        program_->atomic_type_table().AtomicTypeForKind(ir::AtomicTypeKind::kFunc);
    return std::make_shared<ir::Constant>(func_type, number);
  }

  if (scanner_.token() != Scanner::kHashSign) throw "expected '@' or '#'";
  scanner_.Next();

  if (scanner_.token() == Scanner::kIdentifier) {
    ir::AtomicType* bool_type =
        program_->atomic_type_table().AtomicTypeForKind(ir::AtomicTypeKind::kBool);
    if (scanner_.string() == "f") {
      if (expected_type != ir::AtomicTypeKind::kBool) throw "unexpected 'f'";

      return std::make_shared<ir::Constant>(bool_type, false);
    } else if (scanner_.string() == "t") {
      if (expected_type != ir::AtomicTypeKind::kBool) throw "unexpected 't'";

      return std::make_shared<ir::Constant>(bool_type, true);
    } else {
      throw "expected number, 't' or 'f'";
    }
  }

  if (scanner_.token() != Scanner::kNumber) throw "expected number, 't' or 'f'";
  int64_t sign = scanner_.sign();
  uint64_t number = scanner_.number();
  scanner_.Next();

  ir::AtomicType* type;
  if (scanner_.token() == Scanner::kColon) {
    scanner_.Next();
    type = ParseType();

    if (expected_type.has_value() && expected_type != type->kind())
      throw "expected: " + ir::ToString(expected_type.value()) + " got: " + type->ToString();
  } else {
    if (!expected_type.has_value()) throw "expected ':'";

    type = program_->atomic_type_table().AtomicTypeForKind(expected_type.value());
  }

  return std::make_shared<ir::Constant>(type, sign * number);
}

// Computed ::= '%' Identifier (':' Type)?
std::shared_ptr<ir::Computed> Parser::ParseComputed(
    std::optional<ir::AtomicTypeKind> expected_type) {
  if (scanner_.token() != Scanner::kPercentSign) throw "expected '%'";
  scanner_.Next();

  if (scanner_.token() != Scanner::kNumber) throw "expected number";
  int64_t number = scanner_.number();
  scanner_.Next();

  ir::AtomicType* type;
  if (scanner_.token() == Scanner::kColon) {
    scanner_.Next();
    type = ParseType();

    if (expected_type.has_value() && expected_type != type->kind())
      throw "expected: " + ir::ToString(expected_type.value()) + " got: " + type->ToString();
  } else {
    if (!expected_type.has_value()) throw "expected ':'";

    type = program_->atomic_type_table().AtomicTypeForKind(expected_type.value());
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
ir::AtomicType* Parser::ParseType() {
  if (scanner_.token() != Scanner::kIdentifier) throw "expected identifier";
  std::string name = scanner_.string();
  scanner_.Next();

  return program_->atomic_type_table().AtomicTypeForKind(ir::ToAtomicTypeKind(name));
}

}  // namespace ir_proc

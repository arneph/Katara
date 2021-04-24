//
//  parser.cc
//  Katara
//
//  Created by Arne Philipeit on 12/29/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include "parser.h"

namespace ir_proc {

ir::Program* Parser::Parse(std::istream& in_stream) {
  Scanner scanner(in_stream);
  return Parse(scanner);
}

ir::Program* Parser::Parse(Scanner& scanner) {
  Parser parser(scanner);

  parser.ParseProg();

  return parser.prog_;
}

Parser::Parser(Scanner& scanner) : scanner_(scanner) {
  scanner_.Next();
  prog_ = new ir::Program();
}
Parser::~Parser() {}

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

  ir::Func* func = prog_->AddFunc(scanner_.number());

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
      ir::Computed arg = ParseComputed();

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
      ir::Type type = ParseType();

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
  for (ir::Block* block : func->blocks()) {
    ir::Instr* last_instr = block->instrs().back();

    if (ir::JumpInstr* jump = dynamic_cast<ir::JumpInstr*>(last_instr)) {
      int64_t child_num = jump->destination().block();
      ir::Block* child = func->GetBlock(child_num);

      func->AddControlFlow(block, child);

    } else if (ir::JumpCondInstr* jump_cond = dynamic_cast<ir::JumpCondInstr*>(last_instr)) {
      int64_t child_a_num = jump_cond->destination_true().block();
      int64_t child_b_num = jump_cond->destination_false().block();
      ir::Block* child_a = func->GetBlock(child_a_num);
      ir::Block* child_b = func->GetBlock(child_b_num);

      func->AddControlFlow(block, child_a);
      func->AddControlFlow(block, child_b);
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
    func->set_entry_block(block);
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
      block->AddInstr(ParseInstr());
    }
  }
}

// Instr ::= InstrResults Idenifier (':' Type)? (Value (',' Value)*)? NL
ir::Instr* Parser::ParseInstr() {
  std::vector<ir::Computed> results = ParseInstrResults();

  if (scanner_.token() != Scanner::kIdentifier) throw "expected '%' or identifier";
  std::string instr_name = scanner_.string();
  scanner_.Next();

  if (instr_name == "mov") {
    if (results.size() != 1) throw "expected one result for mov instruction";

    return ParseMovInstr(results.at(0));

  } else if (instr_name == "phi") {
    if (results.size() != 1) throw "expected one result for phi instruction";

    return ParsePhiInstr(results.at(0));

  } else if (ir::is_unary_al_operation_string(instr_name)) {
    ir::UnaryALOperation op = ir::to_unary_al_operation(instr_name);

    if (results.size() != 1) throw "expected one result for unary al instruction";

    return ParseUnaryALInstr(results.at(0), op);

  } else if (ir::is_binary_al_operation_string(instr_name)) {
    ir::BinaryALOperation op = ir::to_binary_al_operation(instr_name);

    if (results.size() != 1) throw "expected one result for binary al instruction";

    return ParseBinaryALInstr(results.at(0), op);

  } else if (ir::is_compare_operation_string(instr_name)) {
    ir::CompareOperation op = ir::to_compare_operation(instr_name);

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
ir::MovInstr* Parser::ParseMovInstr(ir::Computed result) {
  ir::Value arg = ParseValue(result.type());

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return new ir::MovInstr(result, arg);
}

// PhiInstr ::= Computed 'phi' InheritedValue (',' InheritedValue)+ NL
ir::PhiInstr* Parser::ParsePhiInstr(ir::Computed result) {
  std::vector<ir::InheritedValue> args;

  ir::InheritedValue first_arg = ParseInheritedValue(result.type());
  args.push_back(first_arg);

  for (;;) {
    if (scanner_.token() == Scanner::kNewLine) {
      scanner_.Next();
      break;
    } else if (scanner_.token() == Scanner::kComma) {
      scanner_.Next();

      ir::InheritedValue arg = ParseInheritedValue(result.type());
      args.push_back(arg);

    } else {
      throw "expected ',' or new line";
    }
  }

  if (args.size() < 2) throw "expected at least two arguments for phi instruction";

  return new ir::PhiInstr(result, args);
}

// UnaryALInstr ::= Computed UnaryALOp ':' Type Value NL
ir::UnaryALInstr* Parser::ParseUnaryALInstr(ir::Computed result, ir::UnaryALOperation op) {
  if (scanner_.token() != Scanner::kColon) throw "expected ':'";
  scanner_.Next();

  ir::Type instr_type = ParseType();
  if (instr_type != result.type()) throw "result type does not match type of unary AL instruction";

  ir::Value operand = ParseValue(instr_type);

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return new ir::UnaryALInstr(op, result, operand);
}

// BinaryALInstr ::= Computed BinaryALOp ':' Type Value ',' Value NL
ir::BinaryALInstr* Parser::ParseBinaryALInstr(ir::Computed result, ir::BinaryALOperation op) {
  if (scanner_.token() != Scanner::kColon) throw "expected ':'";
  scanner_.Next();

  ir::Type instr_type = ParseType();
  if (instr_type != result.type()) throw "result type does not match type of binary AL instruction";

  ir::Value operand_a = ParseValue(instr_type);

  if (scanner_.token() != Scanner::kComma) throw "expected ','";
  scanner_.Next();

  ir::Value operand_b = ParseValue(instr_type);

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return new ir::BinaryALInstr(op, result, operand_a, operand_b);
}

// CompareInstr ::= Computed CompareOp ':' Type Value ',' Value NL
ir::CompareInstr* Parser::ParseCompareInstr(ir::Computed result, ir::CompareOperation op) {
  if (scanner_.token() != Scanner::kColon) throw "expected ':'";
  scanner_.Next();

  ir::Type instr_type = ParseType();
  ir::Value operand_a = ParseValue(instr_type);

  if (scanner_.token() != Scanner::kComma) throw "expected ','";
  scanner_.Next();

  ir::Value operand_b = ParseValue(instr_type);

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return new ir::CompareInstr(op, result, operand_a, operand_b);
}

// JumpInstr ::= 'jmp' BlockValue NL
ir::JumpInstr* Parser::ParseJumpInstr() {
  ir::BlockValue destionation = ParseBlockValue();

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return new ir::JumpInstr(destionation);
}

// JumpCondInstr ::= 'jcc' Value ',' BlockValue ',' BlockValue NL
ir::JumpCondInstr* Parser::ParseJumpCondInstr() {
  ir::Value condition = ParseValue(ir::Type::kBool);

  if (scanner_.token() != Scanner::kComma) throw "expected ','";
  scanner_.Next();

  ir::BlockValue destination_true = ParseBlockValue();

  if (scanner_.token() != Scanner::kComma) throw "expected ','";
  scanner_.Next();

  ir::BlockValue destination_false = ParseBlockValue();

  if (scanner_.token() != Scanner::kNewLine) throw "expected new line";
  scanner_.Next();

  return new ir::JumpCondInstr(condition, destination_true, destination_false);
}

// CallInstr ::= (Computed (',' Computed)* '=')?
//               'call' Value (',' Value)* NL
ir::CallInstr* Parser::ParseCallInstr(std::vector<ir::Computed> results) {
  ir::Value func = ParseValue(ir::Type::kFunc);

  std::vector<ir::Value> args;

  for (;;) {
    if (scanner_.token() == Scanner::kNewLine) {
      scanner_.Next();
      break;
    } else if (scanner_.token() == Scanner::kComma) {
      scanner_.Next();

      ir::Value arg = ParseValue();
      args.push_back(arg);

    } else {
      throw "expected ',' or NL";
    }
  }

  return new ir::CallInstr(func, results, args);
}

ir::ReturnInstr* Parser::ParseReturnInstr() {
  std::vector<ir::Value> args;

  if (scanner_.token() == Scanner::kNewLine) {
    scanner_.Next();

    return new ir::ReturnInstr(args);
  }

  ir::Value first_arg = ParseValue();
  args.push_back(first_arg);

  for (;;) {
    if (scanner_.token() == Scanner::kNewLine) {
      scanner_.Next();
      break;

    } else if (scanner_.token() == Scanner::kComma) {
      scanner_.Next();

      ir::Value arg = ParseValue();
      args.push_back(arg);

    } else {
      throw "expected ',' or new line";
    }
  }

  return new ir::ReturnInstr(args);
}

// InstrResults ::= (Computed (',' Computed)* '=')?
std::vector<ir::Computed> Parser::ParseInstrResults() {
  std::vector<ir::Computed> results;

  if (scanner_.token() == Scanner::kPercentSign) {
    for (;;) {
      ir::Computed result = ParseComputed();

      results.push_back(result);

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
ir::InheritedValue Parser::ParseInheritedValue(ir::Type expected_type) {
  ir::Value value = ParseValue(expected_type);
  ir::BlockValue origin = ParseBlockValue();

  return ir::InheritedValue(value, origin);
}

// Value ::= (Constant | Computed | BlockValue)
ir::Value Parser::ParseValue(ir::Type expected_type) {
  switch (scanner_.token()) {
    case Scanner::kAtSign:
    case Scanner::kHashSign:
      return ParseConstant(expected_type);
      break;
    case Scanner::kPercentSign:
      return ParseComputed(expected_type);
      break;
    case Scanner::kCurlyBracketOpen:
      return ParseBlockValue();
      break;
    default:
      throw "expected '#', '%', '{', or '@'";
  }
}

// Constant ::= '@' Number
//              | '#t' | '#f'
//              | '#' Number (':' Type)?
ir::Constant Parser::ParseConstant(ir::Type expected_type) {
  if (scanner_.token() == Scanner::kAtSign) {
    if (expected_type != ir::Type::kFunc) throw "unexpected '@'";
    scanner_.Next();

    if (scanner_.token() != Scanner::kNumber) throw "expected number";
    int64_t number = scanner_.number();
    scanner_.Next();

    return ir::Constant(ir::Type::kFunc, number);
  }

  if (scanner_.token() != Scanner::kHashSign) throw "expected '@' or '#'";
  scanner_.Next();

  if (scanner_.token() == Scanner::kIdentifier) {
    if (scanner_.string() == "f") {
      if (expected_type != ir::Type::kBool) throw "unexpected 'f'";

      return ir::Constant(ir::Type::kBool, false);
    } else if (scanner_.string() == "t") {
      if (expected_type != ir::Type::kBool) throw "unexpected 't'";

      return ir::Constant(ir::Type::kBool, true);
    } else {
      throw "expected number, 't' or 'f'";
    }
  }

  if (scanner_.token() != Scanner::kNumber) throw "expected number, 't' or 'f'";
  int64_t sign = scanner_.sign();
  uint64_t number = scanner_.number();
  scanner_.Next();

  ir::Type type;
  if (scanner_.token() == Scanner::kColon) {
    scanner_.Next();
    type = ParseType();

    if (expected_type != ir::Type::kUnknown && expected_type != type)
      throw "expected: " + ir::to_string(expected_type) + " got: " + ir::to_string(type);
  } else {
    if (expected_type == ir::Type::kUnknown) throw "expected ':'";

    type = expected_type;
  }

  return ir::Constant(type, sign * number);
}

// Computed ::= '%' Identifier (':' Type)?
ir::Computed Parser::ParseComputed(ir::Type expected_type) {
  if (scanner_.token() != Scanner::kPercentSign) throw "expected '%'";
  scanner_.Next();

  if (scanner_.token() != Scanner::kNumber) throw "expected number";
  int64_t number = scanner_.number();
  scanner_.Next();

  ir::Type type;
  if (scanner_.token() == Scanner::kColon) {
    scanner_.Next();
    type = ParseType();

    if (expected_type != ir::Type::kUnknown && expected_type != type)
      throw "expected: " + ir::to_string(expected_type) + " got: " + ir::to_string(type);
  } else {
    if (expected_type == ir::Type::kUnknown) throw "expected ':'";

    type = expected_type;
  }

  return ir::Computed(type, number);
}

// BlockValue ::= '{' Number '}'
ir::BlockValue Parser::ParseBlockValue() {
  if (scanner_.token() != Scanner::kCurlyBracketOpen) throw "expected '{'";
  scanner_.Next();

  if (scanner_.token() != Scanner::kNumber) throw "expected number";
  int64_t number = scanner_.number();
  scanner_.Next();

  if (scanner_.token() != Scanner::kCurlyBracketClose) throw "expected '}'";
  scanner_.Next();

  return ir::BlockValue(number);
}

// Type ::= Identifier
ir::Type Parser::ParseType() {
  if (scanner_.token() != Scanner::kIdentifier) throw "expected identifier";
  std::string name = scanner_.string();
  scanner_.Next();

  return ir::to_type(name);
}

}  // namespace ir_proc

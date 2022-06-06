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
  std::shared_ptr<ir::Computed> copied_shared_pointer = ParseComputed(result->type());
  scanner().ConsumeToken(::ir_serialization::Scanner::kComma);

  std::shared_ptr<ir::Value> pointer_offset = ParseValue(ir::i64());
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::CopySharedPointerInstr>(result, copied_shared_pointer,
                                                          pointer_offset);
}

std::unique_ptr<ir_ext::DeleteSharedPointerInstr> FuncParser::ParseDeleteSharedInstr() {
  std::shared_ptr<ir::Computed> deleted_shared_pointer = ParseComputed(nullptr);
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::DeleteSharedPointerInstr>(deleted_shared_pointer);
}

std::unique_ptr<ir_ext::MakeUniquePointerInstr> FuncParser::ParseMakeUniqueInstr(
    std::shared_ptr<ir::Computed> result) {
  scanner().ConsumeToken(::ir_serialization::Scanner::kNewLine);

  return std::make_unique<ir_ext::MakeUniquePointerInstr>(result);
}

std::unique_ptr<ir_ext::DeleteUniquePointerInstr> FuncParser::ParseDeleteUniqueInstr() {
  std::shared_ptr<ir::Computed> deleted_unique_pointer = ParseComputed(nullptr);
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

std::shared_ptr<ir::Constant> FuncParser::ParseConstant(const ir::Type* expected_type) {
  if (scanner().token() == ::ir_serialization::Scanner::kString) {
    return ParseStringConstant();
  } else {
    return ::ir_serialization::FuncParser::ParseConstant(expected_type);
  }
}

std::shared_ptr<ir_ext::StringConstant> FuncParser::ParseStringConstant() {
  if (scanner().token() != ::ir_serialization::Scanner::kString) {
    common::fail(scanner().PositionString() + ": expected string constant");
  }
  std::string str = scanner().token_string();
  scanner().Next();

  return std::make_shared<ir_ext::StringConstant>(str);
}

const ir::Type* FuncParser::ParseType() {
  if (scanner().token() == ::ir_serialization::Scanner::kIdentifier) {
    std::string name = scanner().token_text();
    if (name == "lshared_ptr") {
      return ParseSharedPointer();
    } else if (name == "lunique_ptr") {
      return ParseUniquePointer();
    } else if (name == "lstr") {
      return ir_ext::string();
    } else if (name == "larray") {
      return ParseArray();
    } else if (name == "lstruct") {
      return ParseStruct();
    } else if (name == "linterface") {
      return ParseInterface();
    } else if (name == "ltypeid") {
      return ir_ext::type_id();
    }
  }
  return ::ir_serialization::FuncParser::ParseType();
}

const ir_ext::SharedPointer* FuncParser::ParseSharedPointer() {
  if (scanner().ConsumeIdentifier() != "lshared_ptr") {
    common::fail(scanner().PositionString() + ": expected lshared_ptr");
  }
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleOpen);
  const ir::Type* element = ParseType();
  scanner().ConsumeToken(::ir_serialization::Scanner::kComma);

  std::string c = scanner().ConsumeIdentifier();
  if (c != "s" && c != "w") {
    common::fail(scanner().PositionString() + ": expected 's' or 'w'");
  }
  bool is_strong = c == "s";
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleClose);

  auto pointer = std::make_unique<ir_ext::SharedPointer>(is_strong, element);
  const ir_ext::SharedPointer* pointer_ptr = pointer.get();
  program()->type_table().AddType(std::move(pointer));
  return pointer_ptr;
}

const ir_ext::UniquePointer* FuncParser::ParseUniquePointer() {
  if (scanner().ConsumeIdentifier() != "lunique_ptr") {
    common::fail(scanner().PositionString() + ": expected lunique_ptr");
  }
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleOpen);
  const ir::Type* element = ParseType();
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleClose);

  auto pointer = std::make_unique<ir_ext::UniquePointer>(element);
  const ir_ext::UniquePointer* pointer_ptr = pointer.get();
  program()->type_table().AddType(std::move(pointer));
  return pointer_ptr;
}

const ir_ext::Array* FuncParser::ParseArray() {
  if (scanner().ConsumeIdentifier() != "larray") {
    common::fail(scanner().PositionString() + ": expected larray");
  }
  ir_ext::ArrayBuilder builder;
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleOpen);
  builder.SetElement(ParseType());
  if (scanner().token() == ::ir_serialization::Scanner::kComma) {
    scanner().ConsumeToken(::ir_serialization::Scanner::kComma);
    builder.SetFixedSize(scanner().ConsumeInt64());
  }
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleClose);

  const ir_ext::Array* array_ptr = builder.Get();
  program()->type_table().AddType(builder.Build());
  return array_ptr;
}

const ir_ext::Struct* FuncParser::ParseStruct() {
  if (scanner().ConsumeIdentifier() != "lstruct") {
    common::fail(scanner().PositionString() + ": expected lstruct");
  }
  if (scanner().token() != ::ir_serialization::Scanner::kAngleOpen) {
    return ir_ext::empty_struct();
  }
  ir_ext::StructBuilder builder;
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleOpen);
  ParseStructField(builder);
  while (scanner().token() == ::ir_serialization::Scanner::kComma) {
    scanner().ConsumeToken(::ir_serialization::Scanner::kComma);
    ParseStructField(builder);
  }
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleClose);

  const ir_ext::Struct* struct_ptr = builder.Get();
  program()->type_table().AddType(builder.Build());
  return struct_ptr;
}

void FuncParser::ParseStructField(ir_ext::StructBuilder& builder) {
  std::string name = scanner().ConsumeIdentifier();
  scanner().ConsumeToken(::ir_serialization::Scanner::kColon);
  const ir::Type* type = ParseType();
  builder.AddField(name, type);
}

const ir_ext::Interface* FuncParser::ParseInterface() {
  if (scanner().ConsumeIdentifier() != "linterface") {
    common::fail(scanner().PositionString() + ": expected linterface");
  }
  if (scanner().token() != ::ir_serialization::Scanner::kAngleOpen) {
    return ir_ext::empty_interface();
  }
  ir_ext::InterfaceBuilder builder;
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleOpen);
  ParseInterfaceMethod(builder);
  while (scanner().token() == ::ir_serialization::Scanner::kComma) {
    scanner().ConsumeToken(::ir_serialization::Scanner::kComma);
    ParseInterfaceMethod(builder);
  }
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleClose);

  const ir_ext::Interface* interface_ptr = builder.Get();
  program()->type_table().AddType(builder.Build());
  return interface_ptr;
}

void FuncParser::ParseInterfaceMethod(ir_ext::InterfaceBuilder& builder) {
  std::string name = scanner().ConsumeIdentifier();
  scanner().ConsumeToken(::ir_serialization::Scanner::kColon);
  scanner().ConsumeToken(::ir_serialization::Scanner::kParenOpen);
  std::vector<const ir::Type*> parameters;
  if (scanner().token() != ::ir_serialization::Scanner::kParenClose) {
    parameters = ParseTypes();
  }
  scanner().ConsumeToken(::ir_serialization::Scanner::kParenClose);
  scanner().ConsumeToken(::ir_serialization::Scanner::kArrow);
  scanner().ConsumeToken(::ir_serialization::Scanner::kParenOpen);
  std::vector<const ir::Type*> results;
  if (scanner().token() != ::ir_serialization::Scanner::kParenClose) {
    results = ParseTypes();
  }
  scanner().ConsumeToken(::ir_serialization::Scanner::kParenClose);
  builder.AddMethod(name, parameters, results);
}

}  // namespace ir_serialization
}  // namespace lang

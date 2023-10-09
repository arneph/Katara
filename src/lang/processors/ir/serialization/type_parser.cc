//
//  type_parser.cc
//  Katara
//
//  Created by Arne Philipeit on 6/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "type_parser.h"

#include "src/common/logging/logging.h"
#include "src/common/positions/positions.h"

namespace lang {
namespace ir_serialization {

using ::common::logging::fail;
using ::common::positions::pos_t;
using ::common::positions::range_t;
using TypeParseResult = ::ir_serialization::TypeParser::TypeParseResult;

TypeParseResult TypeParser::ParseType() {
  if (scanner().token() == ::ir_serialization::Scanner::kIdentifier) {
    range_t name_range = scanner().token_range();
    std::string name = scanner().token_text();
    if (name == "lshared_ptr") {
      return ParseSharedPointer();
    } else if (name == "lunique_ptr") {
      return ParseUniquePointer();
    } else if (name == "lstr") {
      return TypeParseResult{
          .type = ir_ext::string(),
          .range = name_range,
      };
    } else if (name == "larray") {
      return ParseArray();
    } else if (name == "lstruct") {
      return ParseStruct();
    } else if (name == "linterface") {
      return ParseInterface();
    } else if (name == "ltypeid") {
      return TypeParseResult{
          .type = ir_ext::type_id(),
          .range = name_range,
      };
    }
  }
  return ::ir_serialization::TypeParser::ParseType();
}

TypeParseResult TypeParser::ParseSharedPointer() {
  pos_t start = scanner().token_start();
  if (scanner().ConsumeIdentifier() != "lshared_ptr") {
    fail("expected lshared_ptr");
  }
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleOpen);
  const auto& [element, element_range] = ParseType();
  scanner().ConsumeToken(::ir_serialization::Scanner::kComma);

  std::string c = scanner().ConsumeIdentifier().value_or("s");
  if (c != "s" && c != "w") {
    fail("expected 's' or 'w'");
  }
  bool is_strong = c == "s";
  pos_t end = scanner().token_end();
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleClose);

  auto pointer = std::make_unique<ir_ext::SharedPointer>(is_strong, element);
  const ir_ext::SharedPointer* pointer_ptr = pointer.get();
  program()->type_table().AddType(std::move(pointer));
  return TypeParseResult{
      .type = pointer_ptr,
      .range = range_t{.start = start, .end = end},
  };
}

TypeParseResult TypeParser::ParseUniquePointer() {
  pos_t start = scanner().token_start();
  if (scanner().ConsumeIdentifier() != "lunique_ptr") {
    fail("expected lunique_ptr");
  }
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleOpen);
  const auto& [element, element_range] = ParseType();
  pos_t end = scanner().token_end();
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleClose);

  auto pointer = std::make_unique<ir_ext::UniquePointer>(element);
  const ir_ext::UniquePointer* pointer_ptr = pointer.get();
  program()->type_table().AddType(std::move(pointer));
  return TypeParseResult{
      .type = pointer_ptr,
      .range = range_t{.start = start, .end = end},
  };
}

TypeParseResult TypeParser::ParseArray() {
  pos_t start = scanner().token_start();
  if (scanner().ConsumeIdentifier() != "larray") {
    fail("expected larray");
  }
  ir_ext::ArrayBuilder builder;
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleOpen);
  const auto& [element, element_range] = ParseType();
  builder.SetElement(element);
  if (scanner().token() == ::ir_serialization::Scanner::kComma) {
    scanner().ConsumeToken(::ir_serialization::Scanner::kComma);
    builder.SetFixedCount(scanner().ConsumeInt64().value_or(0));
  }
  pos_t end = scanner().token_end();
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleClose);

  const ir_ext::Array* array_ptr = builder.Get();
  program()->type_table().AddType(builder.Build());
  return TypeParseResult{
      .type = array_ptr,
      .range = range_t{.start = start, .end = end},
  };
}

TypeParseResult TypeParser::ParseStruct() {
  range_t lstruct_range = scanner().token_range();
  if (scanner().ConsumeIdentifier() != "lstruct") {
    fail("expected lstruct");
  }
  if (scanner().token() != ::ir_serialization::Scanner::kAngleOpen) {
    return TypeParseResult{
        .type = ir_ext::empty_struct(),
        .range = lstruct_range,
    };
  }
  ir_ext::StructBuilder builder;
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleOpen);
  ParseStructField(builder);
  while (scanner().token() == ::ir_serialization::Scanner::kComma) {
    scanner().ConsumeToken(::ir_serialization::Scanner::kComma);
    ParseStructField(builder);
  }
  pos_t end = scanner().token_end();
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleClose);

  const ir_ext::Struct* struct_ptr = builder.Get();
  program()->type_table().AddType(builder.Build());
  return TypeParseResult{
      .type = struct_ptr,
      .range = range_t{.start = lstruct_range.start, .end = end},
  };
}

void TypeParser::ParseStructField(ir_ext::StructBuilder& builder) {
  if (scanner().token() != ::ir_serialization::Scanner::kIdentifier) {
    scanner().AddErrorForUnexpectedToken({::ir_serialization::Scanner::kIdentifier});
    scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kComma});
    return;
  }
  std::string name = scanner().ConsumeIdentifier().value();
  scanner().ConsumeToken(::ir_serialization::Scanner::kColon);
  const auto& [type, type_range] = ParseType();
  builder.AddField(name, type);
}

TypeParseResult TypeParser::ParseInterface() {
  range_t linterface_range = scanner().token_range();
  if (scanner().ConsumeIdentifier() != "linterface") {
    fail("expected linterface");
  }
  if (scanner().token() != ::ir_serialization::Scanner::kAngleOpen) {
    return TypeParseResult{
        .type = ir_ext::empty_interface(),
        .range = linterface_range,
    };
  }
  ir_ext::InterfaceBuilder builder;
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleOpen);
  ParseInterfaceMethod(builder);
  while (scanner().token() == ::ir_serialization::Scanner::kComma) {
    scanner().ConsumeToken(::ir_serialization::Scanner::kComma);
    ParseInterfaceMethod(builder);
  }
  pos_t end = scanner().token_end();
  scanner().ConsumeToken(::ir_serialization::Scanner::kAngleClose);

  const ir_ext::Interface* interface_ptr = builder.Get();
  program()->type_table().AddType(builder.Build());
  return TypeParseResult{
      .type = interface_ptr,
      .range = range_t{.start = linterface_range.start, .end = end},
  };
}

void TypeParser::ParseInterfaceMethod(ir_ext::InterfaceBuilder& builder) {
  if (scanner().token() != ::ir_serialization::Scanner::kIdentifier) {
    scanner().AddErrorForUnexpectedToken({::ir_serialization::Scanner::kIdentifier});
    scanner().SkipPastTokenSequence({::ir_serialization::Scanner::kComma});
    return;
  }
  std::string name = scanner().ConsumeIdentifier().value();
  scanner().ConsumeToken(::ir_serialization::Scanner::kColon);
  scanner().ConsumeToken(::ir_serialization::Scanner::kParenOpen);
  std::vector<const ir::Type*> parameters;
  if (scanner().token() != ::ir_serialization::Scanner::kParenClose) {
    const auto& [types, type_ranges, range] = ParseTypes();
    parameters = types;
  }
  scanner().ConsumeToken(::ir_serialization::Scanner::kParenClose);
  scanner().ConsumeToken(::ir_serialization::Scanner::kArrow);
  scanner().ConsumeToken(::ir_serialization::Scanner::kParenOpen);
  std::vector<const ir::Type*> results;
  if (scanner().token() != ::ir_serialization::Scanner::kParenClose) {
    const auto& [types, type_ranges, range] = ParseTypes();
    results = types;
  }
  scanner().ConsumeToken(::ir_serialization::Scanner::kParenClose);
  builder.AddMethod(name, parameters, results);
}

}  // namespace ir_serialization
}  // namespace lang

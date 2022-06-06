//
//  parse.h
//  Katara
//
//  Created by Arne Philipeit on 3/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_parse_h
#define ir_serialization_parse_h

#include <istream>
#include <memory>
#include <sstream>
#include <string>

#include "src/common/logging/logging.h"
#include "src/ir/representation/program.h"
#include "src/ir/serialization/constant_parser.h"
#include "src/ir/serialization/func_parser.h"
#include "src/ir/serialization/scanner.h"
#include "src/ir/serialization/type_parser.h"

namespace ir_serialization {

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
std::unique_ptr<ir::Program> ParseProgram(std::istream& in_stream) {
  Scanner scanner(in_stream);
  scanner.Next();

  // Program ::= (Func | NL)*
  auto program = std::make_unique<ir::Program>();
  TypeParser type_parser(scanner, program.get());
  ConstantParser constant_parser(scanner, &type_parser, program.get());
  for (;;) {
    if (scanner.token() == Scanner::kNewLine) {
      scanner.Next();
    } else if (scanner.token() == Scanner::kAtSign) {
      FuncParser func_parser(scanner, &type_parser, &constant_parser, program.get());
      func_parser.ParseFunc();
    } else if (scanner.token() == Scanner::kEoF) {
      break;
    } else {
      common::fail(scanner.PositionString() + ": expected new line, '@', or end of file");
    }
  }

  return program;
}

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
std::unique_ptr<ir::Program> ParseProgram(std::string text) {
  std::stringstream ss;
  ss << text;
  return ParseProgram<TypeParser, ConstantParser, FuncParser>(ss);
}

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
ir::Func* ParseFunc(ir::Program* program, std::istream& in_stream) {
  Scanner scanner(in_stream);
  scanner.Next();

  TypeParser type_parser(scanner, program);
  ConstantParser constant_parser(scanner, &type_parser, program);
  FuncParser func_parser(scanner, &type_parser, &constant_parser, program);
  return func_parser.ParseFunc();
}

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
ir::Func* ParseFunc(ir::Program* program, std::string text) {
  std::stringstream ss;
  ss << text;
  return ParseFunc<TypeParser, ConstantParser, FuncParser>(program, ss);
}

}  // namespace ir_serialization

#endif /* ir_serialization_parse_h */

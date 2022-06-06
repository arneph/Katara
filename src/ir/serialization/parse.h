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
#include "src/ir/serialization/func_parser.h"
#include "src/ir/serialization/scanner.h"

namespace ir_serialization {

template <typename FuncParser = FuncParser>
std::unique_ptr<ir::Program> ParseProgram(std::istream& in_stream) {
  Scanner scanner(in_stream);
  scanner.Next();

  // Program ::= (Func | NL)*
  auto program = std::make_unique<ir::Program>();
  for (;;) {
    if (scanner.token() == Scanner::kNewLine) {
      scanner.Next();
    } else if (scanner.token() == Scanner::kAtSign) {
      FuncParser func_parser(scanner, program.get());
      func_parser.ParseFunc();
    } else if (scanner.token() == Scanner::kEoF) {
      break;
    } else {
      common::fail(scanner.PositionString() + ": expected new line, '@', or end of file");
    }
  }

  return program;
}

template <typename FuncParser = FuncParser>
std::unique_ptr<ir::Program> ParseProgram(std::string text) {
  std::stringstream ss;
  ss << text;
  return ParseProgram<FuncParser>(ss);
}

template <typename FuncParser = FuncParser>
ir::Func* ParseFunc(ir::Program* program, std::istream& in_stream) {
  Scanner scanner(in_stream);
  scanner.Next();

  FuncParser func_parser(scanner, program);
  return func_parser.ParseFunc();
}

template <typename FuncParser = FuncParser>
ir::Func* ParseFunc(ir::Program* program, std::string text) {
  std::stringstream ss;
  ss << text;
  return ParseFunc<FuncParser>(program, ss);
}

}  // namespace ir_serialization

#endif /* ir_serialization_parse_h */

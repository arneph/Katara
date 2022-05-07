//
//  parse.cc
//  Katara
//
//  Created by Arne Philipeit on 3/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "parse.h"

#include <sstream>

#include "src/common/logging/logging.h"
#include "src/ir/serialization/func_parser.h"
#include "src/ir/serialization/scanner.h"

namespace ir_serialization {

std::unique_ptr<ir::Program> ParseProgram(std::istream& in_stream) {
  Scanner scanner(in_stream);
  scanner.Next();

  // Program ::= (Func | NL)*
  auto program = std::make_unique<ir::Program>();
  for (;;) {
    if (scanner.token() == Scanner::kNewLine) {
      scanner.Next();
    } else if (scanner.token() == Scanner::kAtSign) {
      FuncParser::Parse(scanner, program.get());
    } else if (scanner.token() == Scanner::kEoF) {
      break;
    } else {
      common::fail(scanner.PositionString() + ": expected new line, '@', or end of file");
    }
  }

  return program;
}

std::unique_ptr<ir::Program> ParseProgram(std::string text) {
  std::stringstream ss;
  ss << text;
  return ParseProgram(ss);
}

ir::Func* ParseFunc(ir::Program* program, std::istream& in_stream) {
  Scanner scanner(in_stream);
  scanner.Next();

  return FuncParser::Parse(scanner, program);
}

ir::Func* ParseFunc(ir::Program* program, std::string text) {
  std::stringstream ss;
  ss << text;
  return ParseFunc(program, ss);
}

}  // namespace ir_serialization

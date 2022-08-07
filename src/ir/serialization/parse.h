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
#include <vector>

#include "src/common/logging/logging.h"
#include "src/ir/representation/program.h"
#include "src/ir/serialization/constant_parser.h"
#include "src/ir/serialization/func_parser.h"
#include "src/ir/serialization/scanner.h"
#include "src/ir/serialization/type_parser.h"

namespace ir_serialization {

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
std::vector<ir::Func*> ParseAdditionalFuncsForProgram(ir::Program* program,
                                                      std::istream& in_stream) {
  Scanner scanner(in_stream);
  scanner.Next();

  int64_t func_num_offset = program->funcs().size();
  TypeParser type_parser(scanner, program);
  ConstantParser constant_parser(scanner, &type_parser, program, func_num_offset);
  std::vector<ir::Func*> parsed_funcs;

  // Program ::= (Func | NL)*
  for (;;) {
    if (scanner.token() == Scanner::kNewLine) {
      scanner.Next();
    } else if (scanner.token() == Scanner::kAtSign) {
      FuncParser func_parser(scanner, &type_parser, &constant_parser, program, func_num_offset);
      parsed_funcs.push_back(func_parser.ParseFunc());
    } else if (scanner.token() == Scanner::kEoF) {
      break;
    } else {
      scanner.FailForUnexpectedToken({Scanner::kNewLine, Scanner::kAtSign, Scanner::kEoF});
    }
  }
  return parsed_funcs;
}

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
std::vector<ir::Func*> ParseAdditionalFuncsForProgram(ir::Program* program, std::string text) {
  std::stringstream ss;
  ss << text;
  return ParseAdditionalFuncsForProgram<TypeParser, ConstantParser, FuncParser>(program, ss);
}

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
std::unique_ptr<ir::Program> ParseProgram(std::istream& in_stream) {
  auto program = std::make_unique<ir::Program>();
  ParseAdditionalFuncsForProgram<TypeParser, ConstantParser, FuncParser>(program.get(), in_stream);
  return program;
}

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
std::unique_ptr<ir::Program> ParseProgram(std::string text) {
  std::stringstream ss;
  ss << text;
  return ParseProgram<TypeParser, ConstantParser, FuncParser>(ss);
}

}  // namespace ir_serialization

#endif /* ir_serialization_parse_h */

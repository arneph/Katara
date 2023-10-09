//
//  parse.h
//  Katara
//
//  Created by Arne Philipeit on 3/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_parse_h
#define ir_serialization_parse_h

#include <memory>
#include <string>
#include <vector>

#include "src/common/logging/logging.h"
#include "src/common/positions/positions.h"
#include "src/ir/issues/issues.h"
#include "src/ir/representation/program.h"
#include "src/ir/serialization/constant_parser.h"
#include "src/ir/serialization/func_parser.h"
#include "src/ir/serialization/positions.h"
#include "src/ir/serialization/scanner.h"
#include "src/ir/serialization/type_parser.h"

namespace ir_serialization {

using ::common::logging::error;
using ::common::logging::fail;

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
std::vector<ir::Func*> ParseAdditionalFuncsForProgram(ir::Program* program,
                                                      ProgramPositions& program_positions,
                                                      common::positions::File* file,
                                                      ir_issues::IssueTracker& issue_tracker) {
  Scanner scanner(file, issue_tracker);
  scanner.Next();

  int64_t func_num_offset = program->funcs().size();
  TypeParser type_parser(scanner, issue_tracker, program);
  ConstantParser constant_parser(scanner, issue_tracker, &type_parser, program, func_num_offset);
  std::vector<ir::Func*> parsed_funcs;

  // Program ::= (Func | NL)*
  for (;;) {
    if (scanner.token() == Scanner::kNewLine) {
      scanner.Next();
    } else if (scanner.token() == Scanner::kAtSign) {
      FuncParser func_parser(scanner, issue_tracker, &type_parser, &constant_parser, program,
                             program_positions, func_num_offset);
      parsed_funcs.push_back(func_parser.ParseFunc());
    } else if (scanner.token() == Scanner::kEoF) {
      break;
    } else {
      scanner.AddErrorForUnexpectedToken({Scanner::kNewLine, Scanner::kAtSign, Scanner::kEoF});
      scanner.SkipPastTokenSequence({Scanner::kNewLine});
    }
  }
  return parsed_funcs;
}

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
std::vector<ir::Func*> ParseAdditionalFuncsForProgram(ir::Program* program,
                                                      common::positions::File* file,
                                                      ir_issues::IssueTracker& issue_tracker) {
  ProgramPositions program_positions;
  return ParseAdditionalFuncsForProgram<TypeParser, ConstantParser, FuncParser>(
      program, program_positions, file, issue_tracker);
}

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
std::vector<ir::Func*> ParseAdditionalFuncsForProgramOrDie(ir::Program* program,
                                                           ProgramPositions& program_positions,
                                                           std::string text) {
  common::positions::FileSet file_set;
  common::positions::File* file = file_set.AddFile("unknown.ir", text);
  ir_issues::IssueTracker issue_tracker(&file_set);
  std::vector<ir::Func*> funcs =
      ParseAdditionalFuncsForProgram<TypeParser, ConstantParser, FuncParser>(
          program, program_positions, file, issue_tracker);
  if (!issue_tracker.issues().empty()) {
    error("Parsing IR failed:");
    issue_tracker.PrintIssues(common::issues::Format::kTerminal, &std::cerr);
    fail("");
  }
  return funcs;
}

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
std::vector<ir::Func*> ParseAdditionalFuncsForProgramOrDie(ir::Program* program, std::string text) {
  ProgramPositions program_positions;
  return ParseAdditionalFuncsForProgramOrDie<TypeParser, ConstantParser, FuncParser>(
      program, program_positions, text);
}

struct ProgramWithPositions {
  std::unique_ptr<ir::Program> program;
  ProgramPositions program_positions;
};

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
ProgramWithPositions ParseProgramWithPositions(common::positions::File* file,
                                               ir_issues::IssueTracker& issue_tracker) {
  auto program = std::make_unique<ir::Program>();
  ProgramPositions program_positions;
  ParseAdditionalFuncsForProgram<TypeParser, ConstantParser, FuncParser>(
      program.get(), program_positions, file, issue_tracker);
  return ProgramWithPositions{
      .program = std::move(program),
      .program_positions = program_positions,
  };
}

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
std::unique_ptr<ir::Program> ParseProgram(common::positions::File* file,
                                          ir_issues::IssueTracker& issue_tracker) {
  auto [program, program_positions] =
      ParseProgramWithPositions<TypeParser, ConstantParser, FuncParser>(file, issue_tracker);
  return std::move(program);
}

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
ProgramWithPositions ParseProgramWithPositionsOrDie(std::string text) {
  common::positions::FileSet file_set;
  common::positions::File* file = file_set.AddFile("unknown.ir", text);
  ir_issues::IssueTracker issue_tracker(&file_set);
  ProgramWithPositions program_with_positions =
      ParseProgramWithPositions<TypeParser, ConstantParser, FuncParser>(file, issue_tracker);
  if (!issue_tracker.issues().empty()) {
    error("Parsing IR failed:");
    issue_tracker.PrintIssues(common::issues::Format::kTerminal, &std::cerr);
    fail("");
  }
  return program_with_positions;
}

template <typename TypeParser = TypeParser, typename ConstantParser = ConstantParser,
          typename FuncParser = FuncParser>
std::unique_ptr<ir::Program> ParseProgramOrDie(std::string text) {
  auto [program, program_positions] =
      ParseProgramWithPositionsOrDie<TypeParser, ConstantParser, FuncParser>(text);
  return std::move(program);
}

}  // namespace ir_serialization

#endif /* ir_serialization_parse_h */

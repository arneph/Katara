//
//  parse.cc
//  Katara
//
//  Created by Arne Philipeit on 8/7/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "parse.h"

#include "src/ir/serialization/parse.h"
#include "src/lang/processors/ir/serialization/constant_parser.h"
#include "src/lang/processors/ir/serialization/func_parser.h"
#include "src/lang/processors/ir/serialization/type_parser.h"

namespace lang {
namespace ir_serialization {

std::unique_ptr<ir::Program> ParseProgram(common::positions::File* file,
                                          ir_issues::IssueTracker& issue_tracker) {
  return ::ir_serialization::ParseProgram<TypeParser, ConstantParser, FuncParser>(file,
                                                                                  issue_tracker);
}

std::unique_ptr<ir::Program> ParseProgramOrDie(std::string text) {
  return ::ir_serialization::ParseProgramOrDie<TypeParser, ConstantParser, FuncParser>(text);
}

std::vector<ir::Func*> ParseAdditionalFuncsForProgram(ir::Program* program,
                                                      common::positions::File* file,
                                                      ir_issues::IssueTracker& issue_tracker) {
  return ::ir_serialization::ParseAdditionalFuncsForProgram<TypeParser, ConstantParser, FuncParser>(
      program, file, issue_tracker);
}

std::vector<ir::Func*> ParseAdditionalFuncsForProgramOrDie(ir::Program* program, std::string text) {
  return ::ir_serialization::ParseAdditionalFuncsForProgramOrDie<TypeParser, ConstantParser,
                                                                 FuncParser>(program, text);
}

}  // namespace ir_serialization
}  // namespace lang

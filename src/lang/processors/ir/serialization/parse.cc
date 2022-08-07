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

std::unique_ptr<ir::Program> ParseProgram(std::istream& in_stream) {
  return ::ir_serialization::ParseProgram<TypeParser, ConstantParser, FuncParser>(in_stream);
}

std::unique_ptr<ir::Program> ParseProgram(std::string text) {
  return ::ir_serialization::ParseProgram<TypeParser, ConstantParser, FuncParser>(text);
}

std::vector<ir::Func*> ParseAdditionalFuncsForProgram(ir::Program* program,
                                                      std::istream& in_stream) {
  return ::ir_serialization::ParseAdditionalFuncsForProgram<TypeParser, ConstantParser, FuncParser>(
      program, in_stream);
}

std::vector<ir::Func*> ParseAdditionalFuncsForProgram(ir::Program* program, std::string text) {
  return ::ir_serialization::ParseAdditionalFuncsForProgram<TypeParser, ConstantParser, FuncParser>(
      program, text);
}

}  // namespace ir_serialization
}  // namespace lang

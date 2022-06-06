//
//  parse.h
//  Katara
//
//  Created by Arne Philipeit on 6/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_serialization_parse_h
#define lang_ir_serialization_parse_h

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

ir::Func* ParseFunc(ir::Program* program, std::istream& in_stream) {
  return ::ir_serialization::ParseFunc<TypeParser, ConstantParser, FuncParser>(program, in_stream);
}

ir::Func* ParseFunc(ir::Program* program, std::string text) {
  return ::ir_serialization::ParseFunc<TypeParser, ConstantParser, FuncParser>(program, text);
}

}  // namespace ir_serialization
}  // namespace lang

#endif /* lang_ir_serialization_parse_h */

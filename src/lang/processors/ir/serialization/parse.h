//
//  parse.h
//  Katara
//
//  Created by Arne Philipeit on 6/6/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_serialization_parse_h
#define lang_ir_serialization_parse_h

#include <istream>
#include <memory>
#include <string>
#include <vector>

#include "src/ir/representation/program.h"

namespace lang {
namespace ir_serialization {

std::unique_ptr<ir::Program> ParseProgram(std::istream& in_stream);
std::unique_ptr<ir::Program> ParseProgram(std::string text);
std::vector<ir::Func*> ParseAdditionalFuncsForProgram(ir::Program* program,
                                                      std::istream& in_stream);
std::vector<ir::Func*> ParseAdditionalFuncsForProgram(ir::Program* program, std::string text);

}  // namespace ir_serialization
}  // namespace lang

#endif /* lang_ir_serialization_parse_h */

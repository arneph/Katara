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
#include <string>

#include "src/ir/representation/program.h"

namespace ir_serialization {

std::unique_ptr<ir::Program> ParseProgram(std::istream& in_stream);
std::unique_ptr<ir::Program> ParseProgram(std::string text);

ir::Func* ParseFunc(ir::Program* program, std::istream& in_stream);
ir::Func* ParseFunc(ir::Program* program, std::string text);

}  // namespace ir_serialization

#endif /* ir_serialization_parse_h */

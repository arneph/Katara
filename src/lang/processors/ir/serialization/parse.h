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

#include "src/common/positions/positions.h"
#include "src/ir/issues/issues.h"
#include "src/ir/representation/program.h"
#include "src/ir/serialization/parse.h"
#include "src/ir/serialization/positions.h"

namespace lang {
namespace ir_serialization {

std::unique_ptr<ir::Program> ParseProgram(common::positions::File* file,
                                          ir_issues::IssueTracker& issue_tracker);
::ir_serialization::ProgramWithPositions ParseProgramWithPositions(
    common::positions::File* file, ir_issues::IssueTracker& issue_tracker);
std::unique_ptr<ir::Program> ParseProgramOrDie(std::string text);
::ir_serialization::ProgramWithPositions ParseProgramWithPositionsOrDie(std::string text);

std::vector<ir::Func*> ParseAdditionalFuncsForProgram(ir::Program* program,
                                                      common::positions::File* file,
                                                      ir_issues::IssueTracker& issue_tracker);
std::vector<ir::Func*> ParseAdditionalFuncsForProgram(
    ir::Program* program, ::ir_serialization::ProgramPositions& program_positions,
    common::positions::File* file, ir_issues::IssueTracker& issue_tracker);
std::vector<ir::Func*> ParseAdditionalFuncsForProgramOrDie(ir::Program* program, std::string text);
std::vector<ir::Func*> ParseAdditionalFuncsForProgramOrDie(
    ir::Program* program, ::ir_serialization::ProgramPositions& program_positions,
    std::string text);

}  // namespace ir_serialization
}  // namespace lang

#endif /* lang_ir_serialization_parse_h */

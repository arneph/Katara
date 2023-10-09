//
//  print.h
//  Katara
//
//  Created by Arne Philipeit on 3/26/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_serialization_print_h
#define ir_serialization_print_h

#include <string>

#include "src/common/positions/positions.h"
#include "src/ir/representation/program.h"
#include "src/ir/serialization/positions.h"

namespace ir_serialization {

std::string PrintProgram(const ir::Program* program);
std::string PrintFunc(const ir::Func* func);
std::string PrintBlock(const ir::Block* block);
std::string PrintInstr(const ir::Instr* instr);

struct FilePrintResults {
  common::positions::File* file;
  ProgramPositions program_positions;
};
FilePrintResults PrintProgramToNewFile(std::string file_name, const ir::Program* program,
                                       common::positions::FileSet& file_set);

}  // namespace ir_serialization

#endif /* ir_serialization_print_h */

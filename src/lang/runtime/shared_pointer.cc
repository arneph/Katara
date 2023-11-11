//
//  shared_pointer.cc
//  Katara
//
//  Created by Arne Philipeit on 10/23/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "shared_pointer.h"

#include <fstream>
#include <sstream>
#include <vector>

#include "src/ir/serialization/positions.h"
#include "src/lang/processors/ir/serialization/parse.h"

namespace lang {
namespace runtime {

SharedPointerLoweringFuncs AddSharedPointerLoweringFuncsToProgram(ir::Program* program) {
  std::ifstream fstream("src/lang/runtime/shared_pointer.ir");
  std::stringstream sstream;
  sstream << fstream.rdbuf();
  ::ir_serialization::ProgramPositions discarded_program_positions;
  std::vector<ir::Func*> funcs = lang::ir_serialization::ParseAdditionalFuncsForProgramOrDie(
      program, discarded_program_positions, sstream.str());
  return SharedPointerLoweringFuncs{
      .make_shared_func_num = funcs.at(0)->number(),
      .strong_copy_shared_func_num = funcs.at(1)->number(),
      .weak_copy_shared_func_num = funcs.at(2)->number(),
      .delete_ptr_to_strong_shared_func_num = funcs.at(3)->number(),
      .delete_strong_shared_func_num = funcs.at(4)->number(),
      .delete_ptr_to_weak_shared_func_num = funcs.at(5)->number(),
      .delete_weak_shared_func_num = funcs.at(6)->number(),
      .validate_weak_shared_func_num = funcs.at(7)->number(),
  };
}

}  // namespace runtime
}  // namespace lang

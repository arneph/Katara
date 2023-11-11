//
//  runtime.cc
//  Katara
//
//  Created by Arne Philipeit on 11/11/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#include "runtime.h"

#include "src/ir/representation/program.h"
#include "src/lang/runtime/shared_pointer.h"

namespace lang {
namespace runtime {

RuntimeFuncs AddRuntimeFuncsToProgram(ir::Program* program) {
  return RuntimeFuncs{
      .shared_pointer_funcs = AddSharedPointerFuncsToProgram(program),
  };
}

}  // namespace runtime
}  // namespace lang

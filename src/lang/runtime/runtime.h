//
//  runtime.h
//  Katara
//
//  Created by Arne Philipeit on 11/11/23.
//  Copyright © 2023 Arne Philipeit. All rights reserved.
//

#ifndef lang_runtime_runtime_h
#define lang_runtime_runtime_h

#include "src/ir/representation/program.h"
#include "src/lang/runtime/shared_pointer.h"

namespace lang {
namespace runtime {

struct RuntimeFuncs {
  SharedPointerFuncs shared_pointer_funcs;
};

RuntimeFuncs AddRuntimeFuncsToProgram(ir::Program* program);

}  // namespace runtime
}  // namespace lang

#endif /* lang_runtime_runtime_h */

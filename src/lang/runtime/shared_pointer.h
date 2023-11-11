//
//  shared_pointer.h
//  Katara
//
//  Created by Arne Philipeit on 10/23/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef lang_runtime_shared_pointer_h
#define lang_runtime_shared_pointer_h

#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"

namespace lang {
namespace runtime {

struct SharedPointerFuncs {
  ir::func_num_t make_shared_func_num;
  ir::func_num_t strong_copy_shared_func_num;
  ir::func_num_t weak_copy_shared_func_num;
  ir::func_num_t delete_ptr_to_strong_shared_func_num;
  ir::func_num_t delete_strong_shared_func_num;
  ir::func_num_t delete_ptr_to_weak_shared_func_num;
  ir::func_num_t delete_weak_shared_func_num;
  ir::func_num_t validate_weak_shared_func_num;
};

SharedPointerFuncs AddSharedPointerFuncsToProgram(ir::Program* program);

}  // namespace runtime
}  // namespace lang

#endif /* lang_runtime_shared_pointer_h */

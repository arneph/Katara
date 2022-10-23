//
//  shared_pointer_impl.h
//  Katara
//
//  Created by Arne Philipeit on 10/23/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_lowerers_shared_pointer_impl_h
#define ir_lowerers_shared_pointer_impl_h

#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"

namespace lang {
namespace ir_lowerers {

struct SharedPointerLoweringFuncs {
  ir::func_num_t make_shared_func_num;
  ir::func_num_t strong_copy_shared_func_num;
  ir::func_num_t weak_copy_shared_func_num;
  ir::func_num_t delete_ptr_to_strong_shared_func_num;
  ir::func_num_t delete_strong_shared_func_num;
  ir::func_num_t delete_ptr_to_weak_shared_func_num;
  ir::func_num_t delete_weak_shared_func_num;
  ir::func_num_t validate_weak_shared_func_num;
};

SharedPointerLoweringFuncs AddSharedPointerLoweringFuncsToProgram(ir::Program* program);

}  // namespace ir_lowerers
}  // namespace lang

#endif /* ir_lowerers_shared_pointer_impl_h */

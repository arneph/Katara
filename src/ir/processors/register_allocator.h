//
//  register_allocator.h
//  Katara
//
//  Created by Arne Philipeit on 1/23/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_proc_register_allocator_h
#define ir_proc_register_allocator_h

#include <unordered_map>
#include <unordered_set>

#include "ir/representation/block.h"
#include "ir/representation/func.h"
#include "ir/representation/instr.h"
#include "ir/representation/value.h"
#include "ir/info/func_live_range_info.h"

namespace ir_proc {

class RegisterAllocator {
 public:
  RegisterAllocator(ir::Func* func, ir_info::InterferenceGraph& interference_graph);
  ~RegisterAllocator();

  void AllocateRegisters();

 private:
  // ir::Func* func_;
  ir_info::InterferenceGraph& graph_;
};

}  // namespace ir_proc

#endif /* ir_proc_register_allocator_h */

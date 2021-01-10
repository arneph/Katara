//
//  phi_resolver.h
//  Katara
//
//  Created by Arne Philipeit on 2/23/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_proc_phi_resolver_h
#define ir_proc_phi_resolver_h

#include "ir/block.h"
#include "ir/func.h"
#include "ir/instr.h"
#include "ir/value.h"

namespace ir_proc {

class PhiResolver {
 public:
  PhiResolver(ir::Func* func);
  ~PhiResolver();

  void ResolvePhis();

 private:
  void ResolvePhisInBlock(ir::Block* block);

  ir::Func* func_;
};

}  // namespace ir_proc

#endif /* ir_proc_phi_resolver_h */

//
//  phi_resolver.cc
//  Katara
//
//  Created by Arne Philipeit on 2/23/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "phi_resolver.h"

namespace ir_proc {

PhiResolver::PhiResolver(ir::Func* func) : func_(func) {}
PhiResolver::~PhiResolver() {}

void PhiResolver::ResolvePhis() {
  for (ir::Block* block : func_->blocks()) {
    ResolvePhisInBlock(block);
  }
}

void PhiResolver::ResolvePhisInBlock(ir::Block* block) {
  for (;;) {
    ir::Instr* instr = block->instrs().front();
    ir::PhiInstr* phi_instr = dynamic_cast<ir::PhiInstr*>(instr);
    if (phi_instr == nullptr) {
      break;
    }

    ir::Computed destination = phi_instr->result();

    for (ir::InheritedValue inherited_value : phi_instr->args()) {
      ir::Value source = inherited_value.value();

      ir::MovInstr* mov_instr = new ir::MovInstr(destination, source);

      ir::BlockValue origin = inherited_value.origin();
      ir::Block* parent = func_->GetBlock(origin.block());

      parent->InsertInstr(parent->instrs().size() - 1, mov_instr);
    }

    block->RemoveInstr(phi_instr);
  }
}

}  // namespace ir_proc

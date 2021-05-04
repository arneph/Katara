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
  for (auto& block : func_->blocks()) {
    ResolvePhisInBlock(block.get());
  }
}

void PhiResolver::ResolvePhisInBlock(ir::Block* block) {
  for (;;) {
    ir::Instr* instr = block->instrs().front().get();
    ir::PhiInstr* phi_instr = dynamic_cast<ir::PhiInstr*>(instr);
    if (phi_instr == nullptr) {
      break;
    }

    const auto& destination = phi_instr->result();

    for (auto& inherited_value : phi_instr->args()) {
      const auto& source = inherited_value->value();

      auto mov_instr = std::make_unique<ir::MovInstr>(destination, source);

      ir::block_num_t origin = inherited_value->origin();
      ir::Block* parent = func_->GetBlock(origin);

      parent->instrs().insert(parent->instrs().end() - 1, std::move(mov_instr));
    }

    block->instrs().erase(block->instrs().begin());
  }
}

}  // namespace ir_proc

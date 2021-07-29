//
//  phi_resolver.cc
//  Katara
//
//  Created by Arne Philipeit on 2/23/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "phi_resolver.h"

namespace ir_processors {
namespace {

void ResolvePhisInBlock(ir::Func* func, ir::Block* block) {
  int phi_count = 0;
  for (const auto& instr : block->instrs()) {
    if (instr->instr_kind() != ir::InstrKind::kPhi) {
      break;
    }
    phi_count++;
    const ir::PhiInstr* phi_instr = static_cast<ir::PhiInstr*>(instr.get());
    std::shared_ptr<ir::Computed> destination = phi_instr->result();

    for (const auto& inherited_value : phi_instr->args()) {
      std::shared_ptr<ir::Value> source = inherited_value->value();
      ir::block_num_t origin = inherited_value->origin();
      ir::Block* parent = func->GetBlock(origin);

      // Insert Mov before last (control flow) instruction:
      parent->instrs().insert(parent->instrs().end() - 1,
                              std::make_unique<ir::MovInstr>(destination, source));
    }
  }
  block->instrs().erase(block->instrs().begin(), block->instrs().begin() + phi_count);
}

}  // namespace

void ResolvePhisInFunc(ir::Func* func) {
  for (auto& block : func->blocks()) {
    ResolvePhisInBlock(func, block.get());
  }
}

}  // namespace ir_processors

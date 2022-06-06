//
//  unique_pointer_lowerer.cc
//  Katara
//
//  Created by Arne Philipeit on 6/2/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "unique_pointer_lowerer.h"

#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/lang/representation/ir_extension/instrs.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"

namespace lang {
namespace ir_lowerers {
namespace {

void LowerUniquePointersInFunc(ir::Func* func) {
  func->ForBlocksInDominanceOrder([&](ir::Block* block) {
    for (auto it = block->instrs().begin(); it != block->instrs().end(); ++it) {
      ir::Instr* old_instr = it->get();
      switch (old_instr->instr_kind()) {
        case ir::InstrKind::kLangMakeUniquePointer: {
          auto make_unique_instr = static_cast<ir_ext::MakeUniquePointerInstr*>(old_instr);
          std::shared_ptr<ir::Value> size = ir::I64Eight();  // TODO: use actual size
          std::shared_ptr<ir::Computed> address = make_unique_instr->result();
          address->set_type(ir::pointer_type());
          it = block->instrs().erase(it);
          it = block->instrs().insert(it, std::make_unique<ir::MallocInstr>(address, size));
          break;
        }
        case ir::InstrKind::kLangDeleteUniquePointer: {
          auto delete_unique_instr = static_cast<ir_ext::DeleteUniquePointerInstr*>(old_instr);
          std::shared_ptr<ir::Computed> address = delete_unique_instr->deleted_unique_pointer();
          address->set_type(ir::pointer_type());
          it = block->instrs().erase(it);
          it = block->instrs().insert(it, std::make_unique<ir::FreeInstr>(address));
          break;
        }
        case ir::InstrKind::kLoad: {
          auto load_instr = static_cast<ir::LoadInstr*>(old_instr);
          ir::Value* address = load_instr->address().get();
          if (address->kind() == ir::Value::Kind::kComputed) {
            static_cast<ir::Computed*>(address)->set_type(ir::pointer_type());
          }
          break;
        }
        case ir::InstrKind::kStore: {
          auto store_instr = static_cast<ir::StoreInstr*>(old_instr);
          ir::Value* address = store_instr->address().get();
          if (address->kind() == ir::Value::Kind::kComputed) {
            static_cast<ir::Computed*>(address)->set_type(ir::pointer_type());
          }
          break;
        }
        default:
          break;
      }
    }
  });
}

}  // namespace

void LowerUniquePointersInProgram(ir::Program* program) {
  for (auto& func : program->funcs()) {
    LowerUniquePointersInFunc(func.get());
  }
}

}  // namespace ir_lowerers
}  // namespace lang

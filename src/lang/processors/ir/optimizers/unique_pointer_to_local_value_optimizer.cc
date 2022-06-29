//
//  unique_pointer_to_local_value_optimizer.cc
//  Katara
//
//  Created by Arne Philipeit on 5/8/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "unique_pointer_to_local_value_optimizer.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "src/ir/analyzers/func_values_builder.h"
#include "src/ir/info/func_values.h"
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
namespace ir_optimizers {
namespace {

bool CanConvertPointer(ir::value_num_t value, const ir_info::FuncValues& func_values) {
  ir::Instr* defining_instr = func_values.GetInstrDefiningValue(value);
  if (defining_instr == nullptr ||
      defining_instr->instr_kind() != ir::InstrKind::kLangMakeUniquePointer) {
    return false;
  }
  auto make_unique_ptr_instr = static_cast<ir_ext::MakeUniquePointerInstr*>(defining_instr);
  if (make_unique_ptr_instr->size() != ir::I64One()) {
    return false;
  }
  for (ir::Instr* using_instr : func_values.GetInstrsUsingValue(value)) {
    switch (using_instr->instr_kind()) {
      case ir::InstrKind::kPhi:   // TODO: support analysis with phis
      case ir::InstrKind::kCall:  // TODO: support analysis across function boundaries
      case ir::InstrKind::kReturn:
        return false;
      default:
        break;
    }
  }
  return true;
}

void ConvertPointerInFunc(ir::value_num_t value_num, ir::Func* func,
                          const ir_info::FuncValues& func_values) {
  std::unordered_set<ir::block_num_t> blocks_requiring_phi;
  std::unordered_map<ir::block_num_t, std::shared_ptr<ir::Value>> element_values;
  func->ForBlocksInDominanceOrder([&](ir::Block* block) {
    std::shared_ptr<ir::Value> element_value = nullptr;
    ir::block_num_t element_value_origin = block->number();
    if (block->parents().size() == 1) {
      ir::Block* parent = func->GetBlock(*block->parents().begin());
      while (element_value == nullptr && parent != nullptr) {
        element_value = element_values.at(parent->number());
        element_value_origin = parent->number();
        parent =
            (parent->parents().size() == 1) ? func->GetBlock(*parent->parents().begin()) : nullptr;
      }
    }

    for (auto it = block->instrs().begin(); it != block->instrs().end(); ++it) {
      ir::Instr* old_instr = it->get();
      switch (old_instr->instr_kind()) {
        case ir::InstrKind::kLangMakeUniquePointer:
          if (func_values.GetInstrDefiningValue(value_num) != old_instr) {
            continue;
          }
          it = block->instrs().erase(it);
          --it;
          break;
        case ir::InstrKind::kLangDeleteUniquePointer:
          if (!func_values.GetInstrsUsingValue(value_num).contains(old_instr)) {
            continue;
          }
          it = block->instrs().erase(it);
          --it;
          break;
        case ir::InstrKind::kLoad: {
          if (!func_values.GetInstrsUsingValue(value_num).contains(old_instr)) {
            continue;
          }
          std::shared_ptr<ir::Computed> loaded_value =
              static_cast<ir::LoadInstr*>(old_instr)->result();
          if (element_value == nullptr) {
            element_value = loaded_value;
            element_values.insert_or_assign(element_value_origin, element_value);
            blocks_requiring_phi.insert(element_value_origin);
            it = block->instrs().erase(it);
            --it;
          } else {
            *it = std::make_unique<ir::MovInstr>(loaded_value, element_value);
          }
          break;
        }
        case ir::InstrKind::kStore:
          if (!func_values.GetInstrsUsingValue(value_num).contains(old_instr)) {
            continue;
          }
          element_value = static_cast<ir::StoreInstr*>(old_instr)->value();
          it = block->instrs().erase(it);
          --it;
          break;
        default:
          break;
      }
    }

    element_values.insert({block->number(), element_value});
  });

  for (ir::block_num_t block_num_requiring_phi : blocks_requiring_phi) {
    ir::Block* block_requiring_phi = func->GetBlock(block_num_requiring_phi);
    std::shared_ptr<ir::Computed> phi_result =
        std::static_pointer_cast<ir::Computed>(element_values.at(block_num_requiring_phi));
    std::vector<std::shared_ptr<ir::InheritedValue>> phi_args;
    phi_args.reserve(block_requiring_phi->parents().size());
    for (ir::block_num_t parent_num : block_requiring_phi->parents()) {
      std::shared_ptr<ir::Value> parent_value = element_values.at(parent_num);
      phi_args.push_back(std::make_shared<ir::InheritedValue>(parent_value, parent_num));
    }
    block_requiring_phi->instrs().insert(block_requiring_phi->instrs().begin(),
                                         std::make_unique<ir::PhiInstr>(phi_result, phi_args));
  }
}

void ConvertPointersInFunc(ir::Func* func) {
  const ir_info::FuncValues func_values = ir_analyzers::FindValuesInFunc(func);

  for (ir::value_num_t value :
       func_values.GetValuesWithTypeKind(ir::TypeKind::kLangUniquePointer)) {
    if (CanConvertPointer(value, func_values)) {
      ConvertPointerInFunc(value, func, func_values);
    }
  }
}

}  // namespace

void ConvertUniquePointersToLocalValuesInProgram(ir::Program* program) {
  for (const std::unique_ptr<ir::Func>& func : program->funcs()) {
    ConvertPointersInFunc(func.get());
  }
}

}  // namespace ir_optimizers
}  // namespace lang

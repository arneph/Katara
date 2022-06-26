//
//  shared_to_unique_pointer_optimizer.cc
//  Katara
//
//  Created by Arne Philipeit on 2/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "shared_to_unique_pointer_optimizer.h"

#include <memory>

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
      defining_instr->instr_kind() != ir::InstrKind::kLangMakeSharedPointer) {
    return false;
  }
  for (ir::Instr* using_instr : func_values.GetInstrsUsingValue(value)) {
    switch (using_instr->instr_kind()) {
      case ir::InstrKind::kLangCopySharedPointer:
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

void ConvertValueFromSharedToUniquePointer(ir::value_num_t value_num,
                                           const ir_info::FuncValues& func_values,
                                           ir::Program* program) {
  auto make_shared_instr =
      static_cast<ir_ext::MakeSharedPointerInstr*>(func_values.GetInstrDefiningValue(value_num));
  ir::Computed* value = make_shared_instr->result().get();
  const ir_ext::SharedPointer* shared_pointer = make_shared_instr->pointer_type();
  const ir_ext::UniquePointer* unique_pointer =
      static_cast<ir_ext::UniquePointer*>(program->type_table().AddType(
          std::make_unique<ir_ext::UniquePointer>(shared_pointer->element())));
  value->set_type(unique_pointer);
}

void ConvertMakeSharedToMakeUniquePointer(std::unique_ptr<ir::Instr>& instr) {
  auto old_instr = static_cast<ir_ext::MakeSharedPointerInstr*>(instr.get());
  instr = std::make_unique<ir_ext::MakeUniquePointerInstr>(old_instr->result(), old_instr->size());
}

void ConvertDeleteSharedToDeleteUniquePointer(std::unique_ptr<ir::Instr>& instr) {
  auto old_instr = static_cast<ir_ext::DeleteSharedPointerInstr*>(instr.get());
  instr = std::make_unique<ir_ext::DeleteUniquePointerInstr>(old_instr->deleted_shared_pointer());
}

void ConvertPointerInFunc(ir::value_num_t value_num, ir::Func* func,
                          const ir_info::FuncValues& func_values, ir::Program* program) {
  ConvertValueFromSharedToUniquePointer(value_num, func_values, program);

  for (auto& block : func->blocks()) {
    for (std::size_t instr_index = 0; instr_index < block->instrs().size(); instr_index++) {
      ir::Instr* old_instr = block->instrs().at(instr_index).get();
      switch (old_instr->instr_kind()) {
        case ir::InstrKind::kLangMakeSharedPointer:
          if (func_values.GetInstrDefiningValue(value_num) != old_instr) {
            continue;
          }
          ConvertMakeSharedToMakeUniquePointer(block->instrs().at(instr_index));
          break;
        case ir::InstrKind::kLangDeleteSharedPointer:
          if (!func_values.GetInstrsUsingValue(value_num).contains(old_instr)) {
            continue;
          }
          ConvertDeleteSharedToDeleteUniquePointer(block->instrs().at(instr_index));
        default:
          break;
      }
    }
  }
}

void ConvertPointersInFunc(ir::Func* func, ir::Program* program) {
  const ir_info::FuncValues func_values = ir_analyzers::FindValuesInFunc(func);

  for (ir::value_num_t value :
       func_values.GetValuesWithTypeKind(ir::TypeKind::kLangSharedPointer)) {
    if (!CanConvertPointer(value, func_values)) {
      continue;
    }
    ConvertPointerInFunc(value, func, func_values, program);
  }
}

}  // namespace

void ConvertSharedToUniquePointersInProgram(ir::Program* program) {
  for (const std::unique_ptr<ir::Func>& func : program->funcs()) {
    ConvertPointersInFunc(func.get(), program);
  }
}

}  // namespace ir_optimizers
}  // namespace lang

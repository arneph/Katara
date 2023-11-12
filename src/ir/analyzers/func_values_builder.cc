//
//  func_values_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 2/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "func_values_builder.h"

#include "src/ir/representation/block.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"

namespace ir_analyzers {

using ir_info::FuncValues;

namespace {

void AddFuncArguments(const ir::Func* func, FuncValues& func_values) {
  for (const std::shared_ptr<ir::Computed>& arg : func->args()) {
    func_values.AddValue(arg.get());
  }
}

void AddInstrValues(ir::Instr* instr, FuncValues& func_values) {
  for (std::shared_ptr<ir::Computed> defined_value : instr->DefinedValues()) {
    func_values.AddValue(defined_value.get());
    func_values.SetInstrDefiningValue(instr, defined_value.get());
  }
  for (std::shared_ptr<ir::Value> used_value : instr->UsedValues()) {
    if (used_value->kind() == ir::Value::Kind::kInherited) {
      used_value = std::static_pointer_cast<ir::InheritedValue>(used_value)->value();
    }
    if (used_value->kind() != ir::Value::Kind::kComputed) {
      continue;
    }
    auto used_computed = static_cast<ir::Computed*>(used_value.get());
    func_values.AddInstrUsingValue(instr, used_computed);
  }
}

}  // namespace

const ir_info::FuncValues FindValuesInFunc(const ir::Func* func) {
  ir_info::FuncValues func_values;
  AddFuncArguments(func, func_values);
  for (const std::unique_ptr<ir::Block>& block : func->blocks()) {
    for (const std::unique_ptr<ir::Instr>& instr : block->instrs()) {
      AddInstrValues(instr.get(), func_values);
    }
  }
  return func_values;
}

}  // namespace ir_analyzers

//
//  func_values.cc
//  Katara
//
//  Created by Arne Philipeit on 2/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "func_values.h"

#include <sstream>

namespace ir_info {

std::unordered_set<ir::value_num_t> FuncValues::GetValuesWithType(const ir::Type* type) const {
  if (auto it = values_with_type_.find(type); it != values_with_type_.end()) {
    return it->second;
  }
  return {};
}

std::unordered_set<ir::value_num_t> FuncValues::GetValuesWithTypeKind(
    ir::TypeKind type_kind) const {
  if (auto it = values_with_type_kind_.find(type_kind); it != values_with_type_kind_.end()) {
    return it->second;
  }
  return {};
}

ir::Instr* FuncValues::GetInstrDefiningValue(ir::value_num_t value) const {
  if (auto it = defining_instrs_.find(value); it != defining_instrs_.end()) {
    return it->second;
  }
  return nullptr;
}

std::unordered_set<ir::Instr*> FuncValues::GetInstrsUsingValue(ir::value_num_t value) const {
  if (auto it = using_instrs_.find(value); it != using_instrs_.end()) {
    return it->second;
  }
  return {};
}

void FuncValues::AddValue(ir::Computed* value) {
  values_.insert(value->number());
  values_with_type_[value->type()].insert(value->number());
  values_with_type_kind_[value->type()->type_kind()].insert(value->number());
}

void FuncValues::SetInstrDefiningValue(ir::Instr* instr, ir::Computed* value) {
  defining_instrs_.insert({value->number(), instr});
}

void FuncValues::AddInstrUsingValue(ir::Instr* instr, ir::Computed* value) {
  using_instrs_[value->number()].insert(instr);
}

}  // namespace ir_info

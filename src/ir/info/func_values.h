//
//  func_values.h
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_info_func_values_h
#define ir_info_func_values_h

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"

namespace ir_info {

class FuncValues {
 public:
  const std::unordered_set<ir::value_num_t>& GetValues() const { return values_; }
  std::unordered_set<ir::value_num_t> GetValuesWithType(const ir::Type* type) const;
  std::unordered_set<ir::value_num_t> GetValuesWithTypeKind(ir::TypeKind type_kind) const;
  ir::Instr* GetInstrDefiningValue(ir::value_num_t value) const;
  std::unordered_set<ir::Instr*> GetInstrsUsingValue(ir::value_num_t value) const;

  void AddValue(ir::Computed* value);
  void SetInstrDefiningValue(ir::Instr* instr, ir::Computed* value);
  void AddInstrUsingValue(ir::Instr* instr, ir::Computed* value);

 private:
  std::unordered_set<ir::value_num_t> values_;
  std::unordered_map<const ir::Type*, std::unordered_set<ir::value_num_t>> values_with_type_;
  std::unordered_map<ir::TypeKind, std::unordered_set<ir::value_num_t>> values_with_type_kind_;
  std::unordered_map<ir::value_num_t, ir::Instr*> defining_instrs_;
  std::unordered_map<ir::value_num_t, std::unordered_set<ir::Instr*>> using_instrs_;
};

}  // namespace ir_info

#endif /* ir_info_func_values_h */

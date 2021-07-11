//
//  values.h
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_info_values_h
#define ir_info_values_h

#include <unordered_map>
#include <unordered_set>

#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"

namespace ir_info {

class Values {
 public:
  ir::Instr* GetDefiningInstr(ir::value_num_t value) const { return defining_instrs_.at(value); }
  void SetDefiningInstr(ir::value_num_t value, ir::Instr* instr) {
    defining_instrs_.insert({value, instr});
  }

  const std::unordered_set<ir::Instr*>& GetUsingInstrs(ir::value_num_t value) const {
    return using_instrs_.at(value);
  }
  void AddUsingInstr(ir::value_num_t value, ir::Instr* instr) {
    using_instrs_[value].insert(instr);
  }

 private:
  std::unordered_map<ir::value_num_t, ir::Instr*> defining_instrs_;
  std::unordered_map<ir::value_num_t, std::unordered_set<ir::Instr*>> using_instrs_;
};

}  // namespace ir_info

#endif /* ir_info_values_h */

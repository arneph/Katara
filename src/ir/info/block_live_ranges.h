//
//  block_live_ranges.h
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_info_block_live_ranges_h
#define ir_info_block_live_ranges_h

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"

namespace ir_info {

class BlockLiveRanges {
 public:
  BlockLiveRanges(const ir::Block* block) : block_(block) {}

  bool HasValue(ir::value_num_t value) const;
  bool HasValueDefinition(ir::value_num_t value) const;
  void AddValueDefinition(ir::value_num_t value, const ir::Instr* instr);
  void AddValueUse(ir::value_num_t value, const ir::Instr* instr);
  void PropagateBackwardsFromExitSet(ir::value_num_t value);

  std::unordered_set<ir::value_num_t> GetEntrySet() const;
  std::unordered_set<ir::value_num_t> GetExitSet() const;
  std::unordered_set<ir::value_num_t> GetLiveSet(const ir::Instr* instr) const;

  std::string ToString() const;

 private:
  struct ValueRange {
    const ir::Instr* start_instr_;
    const ir::Instr* end_instr_;
  };

  bool InstrsAreOrdered(const ir::Instr* instr_a, const ir::Instr* instr_b) const;
  bool InstrIsInRange(const ir::Instr* instr, const ValueRange& range) const;

  const ir::Block* block_;
  std::unordered_map<ir::value_num_t, ValueRange> value_ranges_;
};

}  // namespace ir_info

#endif /* ir_info_block_live_ranges_h */

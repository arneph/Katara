//
//  block_live_range_info.h
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_info_block_live_range_info_h
#define ir_info_block_live_range_info_h

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ir/block.h"
#include "ir/instr.h"
#include "ir/value.h"
#include "ir_info/interference_graph.h"

namespace ir_info {

class BlockLiveRangeInfo {
 public:
  BlockLiveRangeInfo(ir::Block* block);
  ~BlockLiveRangeInfo();

  bool HasValue(ir::Computed value) const;
  bool HasValueDefinition(ir::Computed value) const;
  void AddValueDefinition(ir::Computed value, int64_t index);
  void AddValueUse(ir::Computed value, int64_t index);
  void PropagateBackwardsFromExitSet(ir::Computed value);

  std::unordered_set<ir::Computed> GetEntrySet() const;
  std::unordered_set<ir::Computed> GetExitSet() const;
  std::unordered_set<ir::Computed> GetLiveSet(int64_t index) const;

  std::string ToString() const;

 private:
  struct ValueRange {
    int64_t start_index_;
    int64_t end_index_;
  };

  const ir::Block* block_;

  std::unordered_map<ir::Computed, ValueRange> value_ranges_;
};

}  // namespace ir_info

#endif /* ir_info_block_live_range_info_h */

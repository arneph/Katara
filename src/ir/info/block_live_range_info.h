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

#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"

namespace ir_info {

class BlockLiveRangeInfo {
 public:
  BlockLiveRangeInfo(ir::Block* block) : block_(block) {}

  bool HasValue(ir::value_num_t value) const;
  bool HasValueDefinition(ir::value_num_t value) const;
  void AddValueDefinition(ir::value_num_t value, int64_t index);
  void AddValueUse(ir::value_num_t value, int64_t index);
  void PropagateBackwardsFromExitSet(ir::value_num_t value);

  std::unordered_set<ir::value_num_t> GetEntrySet() const;
  std::unordered_set<ir::value_num_t> GetExitSet() const;
  std::unordered_set<ir::value_num_t> GetLiveSet(int64_t index) const;

  std::string ToString() const;

 private:
  struct ValueRange {
    int64_t start_index_;
    int64_t end_index_;
  };

  const ir::Block* block_;

  std::unordered_map<ir::value_num_t, ValueRange> value_ranges_;
};

}  // namespace ir_info

#endif /* ir_info_block_live_range_info_h */

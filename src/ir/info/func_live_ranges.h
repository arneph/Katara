//
//  func_live_ranges.h
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_info_func_live_ranges_h
#define ir_info_func_live_ranges_h

#include <string>
#include <unordered_map>

#include "src/ir/info/block_live_ranges.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"

namespace ir_info {

class FuncLiveRanges {
 public:
  FuncLiveRanges(const ir::Func* func);

  BlockLiveRanges& GetBlockLiveRanges(ir::block_num_t bnum);

  std::string ToString() const;

 private:
  const ir::Func* func_;

  std::unordered_map<ir::block_num_t, BlockLiveRanges> block_live_ranges_;
};

}  // namespace ir_info

#endif /* ir_info_func_live_ranges_h */

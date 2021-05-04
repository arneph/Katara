//
//  func_live_range_info.h
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_info_func_live_range_info_h
#define ir_info_func_live_range_info_h

#include <string>
#include <unordered_map>

#include "ir/info/block_live_range_info.h"
#include "ir/info/interference_graph.h"
#include "ir/representation/block.h"
#include "ir/representation/func.h"

namespace ir_info {

class FuncLiveRangeInfo {
 public:
  FuncLiveRangeInfo(ir::Func* func_);

  BlockLiveRangeInfo& GetBlockLiveRangeInfo(ir::block_num_t bnum);

  std::string ToString() const;

 private:
  const ir::Func* func_;

  std::unordered_map<ir::block_num_t, BlockLiveRangeInfo> block_live_range_infos_;
};

}  // namespace ir_info

#endif /* ir_info_func_live_range_info_h */

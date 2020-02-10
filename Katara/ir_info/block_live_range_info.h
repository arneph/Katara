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
#include <unordered_set>

#include "ir/block.h"
#include "ir/instr.h"
#include "ir/value.h"

#include "ir_info/interference_graph.h"

namespace ir_info {

class FuncLiveRangeInfo;

class BlockLiveRangeInfo {
public:
    BlockLiveRangeInfo(ir::Block *block);
    ~BlockLiveRangeInfo();
    
    std::unordered_set<ir::Computed>& definitions();
    std::unordered_set<ir::Computed>& entry_set();
    std::unordered_set<ir::Computed>& exit_set();
    
    std::string ToString() const;
    
private:
    const ir::Block *block_;
    
    std::unordered_set<ir::Computed> definitions_;
    std::unordered_set<ir::Computed> entry_set_;
    std::unordered_set<ir::Computed> exit_set_;
};

}

#endif /* ir_info_block_live_range_info_h */

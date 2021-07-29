//
//  live_range_analyzer.h
//  Katara
//
//  Created by Arne Philipeit on 1/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_analyzers_live_range_analyzer_h
#define ir_analyzers_live_range_analyzer_h

#include "src/ir/info/block_live_ranges.h"
#include "src/ir/info/func_live_ranges.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"

namespace ir_analyzers {

const ir_info::FuncLiveRanges FindLiveRangesForFunc(const ir::Func* func);

}  // namespace ir_analyzers

#endif /* ir_analyzers_live_range_analyzer_h */

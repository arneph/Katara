//
//  interference_graph_builder.h
//  Katara
//
//  Created by Arne Philipeit on 7/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_proc_interference_graph_builder_h
#define ir_proc_interference_graph_builder_h

#include "src/ir/info/block_live_ranges.h"
#include "src/ir/info/func_live_ranges.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"

namespace ir_proc {

const ir_info::InterferenceGraph BuildInterferenceGraphForFunc(
    const ir::Func* func, const ir_info::FuncLiveRanges live_ranges);

}

#endif /* ir_proc_interference_graph_builder_h */

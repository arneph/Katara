//
//  interference_graph_builder.h
//  Katara
//
//  Created by Arne Philipeit on 7/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_analyzers_interference_graph_builder_h
#define ir_analyzers_interference_graph_builder_h

#include "src/ir/info/block_live_ranges.h"
#include "src/ir/info/func_live_ranges.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"

namespace ir_analyzers {

const ir_info::InterferenceGraph BuildInterferenceGraphForFunc(
    const ir::Func* func, const ir_info::FuncLiveRanges live_ranges);

}

#endif /* ir_analyzers_interference_graph_builder_h */

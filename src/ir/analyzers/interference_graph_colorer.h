//
//  interference_graph_colorer.h
//  Katara
//
//  Created by Arne Philipeit on 1/23/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_analyzers_interference_graph_colorer_h
#define ir_analyzers_interference_graph_colorer_h

#include <unordered_map>
#include <unordered_set>

#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/values.h"

namespace ir_analyzers {

const ir_info::InterferenceGraphColors ColorInterferenceGraph(
    const ir_info::InterferenceGraph& graph,
    const ir_info::InterferenceGraphColors& preferred_colors);

}  // namespace ir_analyzers

#endif /* ir_analyzers_interference_graph_colorer_h */

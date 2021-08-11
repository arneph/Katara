//
//  ir_translator.h
//  Katara
//
//  Created by Arne Philipeit on 2/15/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_h
#define ir_to_x86_64_translator_h

#include <memory>
#include <unordered_map>

#include "src/ir/info/func_live_ranges.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/x86_64/program.h"

namespace ir_to_x86_64_translator {

std::unique_ptr<x86_64::Program> Translate(
    const ir::Program* program,
    const std::unordered_map<ir::func_num_t, const ir_info::FuncLiveRanges>& live_ranges,
    const std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraph>&
        interference_graphs);

}  // namespace ir_to_x86_64_translator

#endif /* ir_to_x86_64_translator_h */

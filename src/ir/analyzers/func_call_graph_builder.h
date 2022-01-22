//
//  func_call_graph_builder.h
//  Katara
//
//  Created by Arne Philipeit on 1/9/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef ir_analyzers_func_call_graph_builder_h
#define ir_analyzers_func_call_graph_builder_h

#include "src/ir/info/func_call_graph.h"
#include "src/ir/representation/program.h"

namespace ir_analyzers {

const ir_info::FuncCallGraph BuildFuncCallGraphForProgram(const ir::Program* program);

}

#endif /* ir_analyzers_func_call_graph_builder_h */

//
//  func_call_graph_optimizer.cc
//  Katara
//
//  Created by Arne Philipeit on 1/22/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "func_call_graph_optimizer.h"

#include "src/ir/analyzers/func_call_graph_builder.h"
#include "src/ir/info/func_call_graph.h"

namespace ir_optimizers {

void RemoveUnusedFunctions(ir::Program* program) {
  ir_info::FuncCallGraph fcg = ir_analyzers::BuildFuncCallGraphForProgram(program);
  ir_info::Component* entry_component = fcg.ComponentOfFunc(program->entry_func_num());
  std::unordered_set<ir::func_num_t> funcs_to_keep =
      fcg.FuncsReachableFromComponent(entry_component);
  std::vector<ir::func_num_t> all_funcs;
  for (const auto& func : program->funcs()) {
    all_funcs.push_back(func->number());
  }
  for (ir::func_num_t func : all_funcs) {
    if (!funcs_to_keep.contains(func)) {
      program->RemoveFunc(func);
    }
  }
}

}  // namespace ir_optimizers

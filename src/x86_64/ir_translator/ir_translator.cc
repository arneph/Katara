//
//  ir_translator.cc
//  Katara
//
//  Created by Arne Philipeit on 2/15/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "ir_translator.h"

#include <string>
#include <vector>

#include "src/ir/representation/func.h"
#include "src/x86_64/func.h"
#include "src/x86_64/ir_translator/func_translator.h"
#include "src/x86_64/ir_translator/register_allocator.h"

namespace ir_to_x86_64_translator {
namespace {

std::vector<x86_64::Func*> PrepareFuncs(ProgramContext& ctx) {
  std::vector<x86_64::Func*> x86_64_funcs;
  x86_64_funcs.reserve(ctx.ir_program()->funcs().size());
  for (auto& ir_func : ctx.ir_program()->funcs()) {
    ir::func_num_t ir_func_num = ir_func->number();
    std::string ir_func_name = ir_func->name();
    if (ir_func_name.empty()) {
      ir_func_name = "Func" + std::to_string(ir_func->number());
    }
    x86_64::Func* x86_64_func = ctx.x86_64_program()->DefineFunc(ir_func_name);

    x86_64_funcs.push_back(x86_64_func);
    ctx.set_x86_64_func_num_for_ir_func_num(ir_func_num, x86_64_func->func_num());
  }
  return x86_64_funcs;
}

}  // namespace

TranslationResults Translate(
    const ir::Program* ir_program,
    const std::unordered_map<ir::func_num_t, const ir_info::FuncLiveRanges>& live_ranges,
    const std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraph>& interference_graphs,
    bool generate_debug_info) {
  auto x86_64_program = std::make_unique<x86_64::Program>();

  x86_64::func_num_t malloc_func_num = x86_64_program->DeclareFunc("malloc");
  x86_64::func_num_t free_func_num = x86_64_program->DeclareFunc("free");
  ProgramContext program_ctx(ir_program, x86_64_program.get(), malloc_func_num, free_func_num);
  std::vector<x86_64::Func*> x86_64_funcs = PrepareFuncs(program_ctx);

  std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraphColors>
      interference_graph_colors = AllocateRegisters(ir_program, interference_graphs);

  for (std::size_t i = 0; i < ir_program->funcs().size(); i++) {
    ir::Func* ir_func = ir_program->funcs().at(i).get();
    ir::func_num_t ir_func_num = ir_func->number();
    x86_64::Func* x86_64_func = x86_64_funcs.at(i);

    FuncContext func_ctx(program_ctx, ir_func, x86_64_func, live_ranges.at(ir_func_num),
                         interference_graphs.at(ir_func_num),
                         interference_graph_colors.at(ir_func_num));
    TranslateFunc(func_ctx);
  }

  TranslationResults results{
      .program = std::move(x86_64_program),
  };
  if (generate_debug_info) {
    results.interference_graph_colors = std::move(interference_graph_colors);
  }

  return results;
}

}  // namespace ir_to_x86_64_translator

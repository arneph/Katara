//
//  build.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "build.h"

#include <unordered_map>

#include "src/cmd/load.h"
#include "src/ir/analyzers/interference_graph_builder.h"
#include "src/ir/analyzers/live_range_analyzer.h"
#include "src/ir/info/func_live_ranges.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/processors/phi_resolver.h"
#include "src/lang/processors/ir_builder/ir_builder.h"
#include "src/lang/processors/ir_lowerers/shared_pointer_lowerer.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/processors/packages/package_manager.h"
#include "src/x86_64/ir_translator/ir_translator.h"

namespace cmd {
namespace {

std::string SubdirNameForFunc(lang::packages::Package* main_pkg, ir::Func* func) {
  return main_pkg->name() + "_@" + std::to_string(func->number()) + "_" + func->name();
}

void GenerateIrDebugInfo(
    ir::Program* program, lang::packages::Package* main_pkg, std::string iter,
    std::unordered_map<ir::func_num_t, const ir_info::FuncLiveRanges>* live_ranges,
    std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraph>* interference_graphs,
    Context* ctx) {
  ctx->WriteToDebugFile(program->ToString(), /* subdir_name= */ "", "ir." + iter + ".txt");

  for (auto& func : program->funcs()) {
    std::string subdir_name = SubdirNameForFunc(main_pkg, func.get());
    std::string file_name = iter;
    common::Graph func_cfg = func->ToControlFlowGraph();
    common::Graph func_dom = func->ToDominatorTree();

    ctx->WriteToDebugFile(func_cfg.ToDotFormat(), subdir_name, file_name + ".cfg.dot");
    ctx->WriteToDebugFile(func_dom.ToDotFormat(), subdir_name, file_name + ".dom.dot");
    if (live_ranges != nullptr) {
      ctx->WriteToDebugFile(live_ranges->at(func->number()).ToString(), subdir_name,
                            file_name + ".live_range_info.txt");
    }
    if (interference_graphs != nullptr) {
      const ir_info::InterferenceGraph& func_interference_graph =
          interference_graphs->at(func->number());
      ctx->WriteToDebugFile(func_interference_graph.ToString(), subdir_name,
                            file_name + ".interference_graph.txt");
      ctx->WriteToDebugFile(func_interference_graph.ToGraph().ToDotFormat(), subdir_name,
                            file_name + ".interference_graph.dot");
    }
  }
}

void GenerateX86_64DebugInfo(
    ir::Program* ir_program, lang::packages::Package* main_pkg,
    std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraph>& interference_graphs,
    ir_to_x86_64_translator::TranslationResults& translation_results, Context* ctx) {
  ctx->WriteToDebugFile(translation_results.program->ToString(), /* subdir_name= */ "",
                        "x86_64.asm.txt");

  for (auto& func : ir_program->funcs()) {
    std::string subdir_name = SubdirNameForFunc(main_pkg, func.get());

    const ir::func_num_t ir_func_num = func->number();
    const x86_64::func_num_t x86_64_func_num =
        translation_results.ir_to_x86_64_func_nums.at(ir_func_num);
    const x86_64::Func* x86_64_func =
        translation_results.program->DefinedFuncWithNumber(x86_64_func_num);
    const ir_info::InterferenceGraph& func_interference_graph =
        interference_graphs.at(func->number());
    const ir_info::InterferenceGraphColors& func_interference_graph_colors =
        translation_results.interference_graph_colors.at(func->number());

    ctx->WriteToDebugFile(x86_64_func->ToString(), subdir_name, "x86_64.asm.txt");
    ctx->WriteToDebugFile(
        func_interference_graph.ToGraph(&func_interference_graph_colors).ToDotFormat(), subdir_name,
        "x86_64.interference_graph.dot");
    ctx->WriteToDebugFile(func_interference_graph_colors.ToString(), subdir_name,
                          "x86_64.colors.txt");
  }
}

}  // namespace

std::variant<BuildResult, ErrorCode> Build(Context* ctx) {
  std::variant<LoadResult, ErrorCode> load_result_or_error = Load(ctx);
  if (std::holds_alternative<ErrorCode>(load_result_or_error)) {
    return std::get<ErrorCode>(load_result_or_error);
  }
  LoadResult& load_result = std::get<LoadResult>(load_result_or_error);
  std::unique_ptr<lang::packages::PackageManager>& pkg_manager = load_result.pkg_manager;

  lang::packages::Package* main_pkg = pkg_manager->GetMainPackage();
  if (main_pkg == nullptr) {
    // TODO: support translating non-main packages to IR
    return kBuildErrorNoMainPackage;
  }

  std::unique_ptr<ir::Program> ir_program =
      lang::ir_builder::IRBuilder::TranslateProgram(main_pkg, pkg_manager->type_info());
  if (ir_program == nullptr) {
    return kBuildErrorTranslationToIRProgramFailed;
  }
  if (ctx->generate_debug_info()) {
    GenerateIrDebugInfo(ir_program.get(), main_pkg, "init", /* live_ranges= */ nullptr,
                        /* interference_graphs= */ nullptr, ctx);
  }

  lang::ir_lowerers::LowerSharedPointersInProgram(ir_program.get());
  std::unordered_map<ir::func_num_t, const ir_info::FuncLiveRanges> live_ranges;
  std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraph> interference_graphs;
  for (auto& func : ir_program->funcs()) {
    const ir_info::FuncLiveRanges func_live_ranges =
        ir_analyzers::FindLiveRangesForFunc(func.get());
    const ir_info::InterferenceGraph func_interference_graph =
        ir_analyzers::BuildInterferenceGraphForFunc(func.get(), func_live_ranges);

    live_ranges.insert({func->number(), func_live_ranges});
    interference_graphs.insert({func->number(), func_interference_graph});
  }
  if (ctx->generate_debug_info()) {
    GenerateIrDebugInfo(ir_program.get(), main_pkg, "lowered", &live_ranges, &interference_graphs,
                        ctx);
  }

  for (auto& func : ir_program->funcs()) {
    ir_processors::ResolvePhisInFunc(func.get());
  }

  ir_to_x86_64_translator::TranslationResults translation_results =
      ir_to_x86_64_translator::Translate(ir_program.get(), live_ranges, interference_graphs,
                                         ctx->generate_debug_info());

  if (ctx->generate_debug_info()) {
    GenerateX86_64DebugInfo(ir_program.get(), main_pkg, interference_graphs, translation_results,
                            ctx);
  }

  return BuildResult{
      .ir_program = std::move(ir_program),
      .x86_64_program = std::move(translation_results.program),
  };
}

}  // namespace cmd

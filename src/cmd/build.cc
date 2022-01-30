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
#include "src/ir/analyzers/func_call_graph_builder.h"
#include "src/ir/analyzers/interference_graph_builder.h"
#include "src/ir/analyzers/live_range_analyzer.h"
#include "src/ir/info/func_call_graph.h"
#include "src/ir/info/func_live_ranges.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/optimizers/func_call_graph_optimizer.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/num_types.h"
#include "src/lang/processors/ir_builder/ir_builder.h"
#include "src/lang/processors/ir_lowerers/shared_pointer_lowerer.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/processors/packages/package_manager.h"

namespace cmd {
namespace {

std::string SubdirNameForFunc(ir::Func* func) {
  return "@" + std::to_string(func->number()) + "_" + func->name();
}

void GenerateIrDebugInfo(ir::Program* program, std::string iter, Context* ctx) {
  ctx->WriteToDebugFile(program->ToString(), /* subdir_name= */ "", "ir." + iter + ".txt");

  const ir_info::FuncCallGraph fcg = ir_analyzers::BuildFuncCallGraphForProgram(program);
  ctx->WriteToDebugFile(fcg.ToGraph(program).ToDotFormat(), /* subdir_name= */ "",
                        "ir." + iter + ".fcg.dot");

  for (auto& func : program->funcs()) {
    std::string subdir_name = SubdirNameForFunc(func.get());
    std::string file_name = iter;

    common::Graph func_cfg = func->ToControlFlowGraph();
    ctx->WriteToDebugFile(func_cfg.ToDotFormat(), subdir_name, file_name + ".cfg.dot");

    common::Graph func_dom = func->ToDominatorTree();
    ctx->WriteToDebugFile(func_dom.ToDotFormat(), subdir_name, file_name + ".dom.dot");

    const ir_info::FuncLiveRanges live_ranges = ir_analyzers::FindLiveRangesForFunc(func.get());
    ctx->WriteToDebugFile(live_ranges.ToString(), subdir_name, file_name + ".live_range_info.txt");

    const ir_info::InterferenceGraph interference_graph =
        ir_analyzers::BuildInterferenceGraphForFunc(func.get(), live_ranges);
    ctx->WriteToDebugFile(interference_graph.ToString(), subdir_name,
                          file_name + ".interference_graph.txt");
    ctx->WriteToDebugFile(interference_graph.ToGraph().ToDotFormat(), subdir_name,
                          file_name + ".interference_graph.dot");
  }
}

std::variant<std::unique_ptr<ir::Program>, ErrorCode> BuildIrProgram(Context* ctx) {
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

  std::unique_ptr<ir::Program> program =
      lang::ir_builder::IRBuilder::TranslateProgram(main_pkg, pkg_manager->type_info());
  if (program == nullptr) {
    return kBuildErrorTranslationToIRProgramFailed;
  }
  if (ctx->generate_debug_info()) {
    GenerateIrDebugInfo(program.get(), "init", ctx);
  }

  return program;
}

void LowerIrProgram(ir::Program* program, Context* ctx) {
  lang::ir_lowerers::LowerSharedPointersInProgram(program);
  if (ctx->generate_debug_info()) {
    GenerateIrDebugInfo(program, "lowered", ctx);
  }
}

void OptimizeIrProgram(ir::Program* program, Context* ctx) {
  ir_optimizers::RemoveUnusedFunctions(program);
  if (ctx->generate_debug_info()) {
    GenerateIrDebugInfo(program, "optimized", ctx);
  }
}

}  // namespace

std::variant<std::unique_ptr<ir::Program>, ErrorCode> Build(Context* ctx) {
  std::variant<std::unique_ptr<ir::Program>, ErrorCode> program_or_error = BuildIrProgram(ctx);
  if (std::holds_alternative<ErrorCode>(program_or_error)) {
    return std::get<ErrorCode>(program_or_error);
  }
  auto ir_program = std::get<std::unique_ptr<ir::Program>>(std::move(program_or_error));

  LowerIrProgram(ir_program.get(), ctx);
  OptimizeIrProgram(ir_program.get(), ctx);

  return std::move(ir_program);
}

}  // namespace cmd

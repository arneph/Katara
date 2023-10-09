//
//  build.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "build.h"

#include <unordered_map>

#include "src/cmd/katara/load.h"
#include "src/common/positions/positions.h"
#include "src/ir/analyzers/func_call_graph_builder.h"
#include "src/ir/analyzers/interference_graph_builder.h"
#include "src/ir/analyzers/live_range_analyzer.h"
#include "src/ir/check/check.h"
#include "src/ir/info/func_call_graph.h"
#include "src/ir/info/func_live_ranges.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/optimizers/func_call_graph_optimizer.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/serialization/print.h"
#include "src/lang/processors/ir/builder/ir_builder.h"
#include "src/lang/processors/ir/check/check.h"
#include "src/lang/processors/ir/lowerers/shared_pointer_lowerer.h"
#include "src/lang/processors/ir/lowerers/unique_pointer_lowerer.h"
#include "src/lang/processors/ir/optimizers/shared_to_unique_pointer_optimizer.h"
#include "src/lang/processors/ir/optimizers/unique_pointer_to_local_value_optimizer.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/processors/packages/package_manager.h"

namespace cmd {
namespace katara {
namespace {

std::string SubdirNameForFunc(ir::Func* func) {
  return "@" + std::to_string(func->number()) + "_" + func->name();
}

void GenerateIrDebugInfo(ir::Program* program, std::string iter, DebugHandler& debug_handler) {
  debug_handler.WriteToDebugFile(ir_serialization::PrintProgram(program), /* subdir_name= */ "",
                                 "ir." + iter + ".txt");

  const ir_info::FuncCallGraph fcg = ir_analyzers::BuildFuncCallGraphForProgram(program);
  debug_handler.WriteToDebugFile(fcg.ToGraph(program).ToDotFormat(), /* subdir_name= */ "",
                                 "ir." + iter + ".fcg.dot");

  for (auto& func : program->funcs()) {
    std::string subdir_name = SubdirNameForFunc(func.get());
    std::string file_name = iter;

    common::graph::Graph func_cfg = func->ToControlFlowGraph();
    debug_handler.WriteToDebugFile(func_cfg.ToDotFormat(), subdir_name, file_name + ".cfg.dot");

    common::graph::Graph func_dom = func->ToDominatorTree();
    debug_handler.WriteToDebugFile(func_dom.ToDotFormat(), subdir_name, file_name + ".dom.dot");

    const ir_info::FuncLiveRanges live_ranges = ir_analyzers::FindLiveRangesForFunc(func.get());
    debug_handler.WriteToDebugFile(live_ranges.ToString(), subdir_name,
                                   file_name + ".live_range_info.txt");

    const ir_info::InterferenceGraph interference_graph =
        ir_analyzers::BuildInterferenceGraphForFunc(func.get(), live_ranges);
    debug_handler.WriteToDebugFile(interference_graph.ToString(), subdir_name,
                                   file_name + ".interference_graph.txt");
    debug_handler.WriteToDebugFile(interference_graph.ToGraph().ToDotFormat(), subdir_name,
                                   file_name + ".interference_graph.dot");
  }
}

std::variant<std::unique_ptr<ir::Program>, ErrorCode> BuildIrProgram(
    std::vector<std::filesystem::path>& paths, DebugHandler& debug_handler, Context* ctx) {
  std::variant<LoadResult, ErrorCode> load_result_or_error = Load(paths, debug_handler, ctx);
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
  if (debug_handler.GenerateDebugInfo()) {
    GenerateIrDebugInfo(program.get(), "init", debug_handler);
  }
  if (debug_handler.CheckIr()) {
    common::positions::FileSet ir_file_set;
    auto [ir_file, program_positions] =
        ::ir_serialization::PrintProgramToNewFile("ir.init.txt", program.get(), ir_file_set);
    ir_issues::IssueTracker issue_tracker(&ir_file_set);
    ::lang::ir_check::CheckProgram(program.get(), program_positions, issue_tracker);
    if (!issue_tracker.issues().empty()) {
      *ctx->stderr() << "init IR program has issues:\n";
      issue_tracker.PrintIssues(common::issues::Format::kTerminal, ctx->stderr());
    }
  }

  return program;
}

void OptimizeIrExtProgram(ir::Program* program, DebugHandler& debug_handler, Context* ctx) {
  lang::ir_optimizers::ConvertSharedToUniquePointersInProgram(program);
  lang::ir_optimizers::ConvertUniquePointersToLocalValuesInProgram(program);
  if (debug_handler.GenerateDebugInfo()) {
    GenerateIrDebugInfo(program, "ext_optimized", debug_handler);
  }
  if (debug_handler.CheckIr()) {
    // TODO: implement lowering for panic and other instructions, then revert to using plain IR
    // checker here.
    common::positions::FileSet ir_file_set;
    auto [ir_file, program_positions] =
        ::ir_serialization::PrintProgramToNewFile("ir.ext_optimized.txt", program, ir_file_set);
    ir_issues::IssueTracker issue_tracker(&ir_file_set);
    ::lang::ir_check::CheckProgram(program, program_positions, issue_tracker);
    if (!issue_tracker.issues().empty()) {
      *ctx->stderr() << "ext_optimized IR program has issues:\n";
      issue_tracker.PrintIssues(common::issues::Format::kTerminal, ctx->stderr());
    }
  }
}

void LowerIrExtProgram(ir::Program* program, DebugHandler& debug_handler, Context* ctx) {
  lang::ir_lowerers::LowerSharedPointersInProgram(program);
  lang::ir_lowerers::LowerUniquePointersInProgram(program);
  if (debug_handler.GenerateDebugInfo()) {
    GenerateIrDebugInfo(program, "lowered", debug_handler);
  }
  if (debug_handler.CheckIr()) {
    // TODO: implement lowering for panic and other instructions, then revert to using plain IR
    // checker here.
    common::positions::FileSet ir_file_set;
    auto [ir_file, program_positions] =
        ::ir_serialization::PrintProgramToNewFile("ir.lowered.txt", program, ir_file_set);
    ir_issues::IssueTracker issue_tracker(&ir_file_set);
    ::lang::ir_check::CheckProgram(program, program_positions, issue_tracker);
    if (!issue_tracker.issues().empty()) {
      *ctx->stderr() << "lowered IR program has issues:\n";
      issue_tracker.PrintIssues(common::issues::Format::kTerminal, ctx->stderr());
    }
  }
}

void OptimizeIrProgram(ir::Program* program, DebugHandler& debug_handler, Context* ctx) {
  ir_optimizers::RemoveUnusedFunctions(program);
  if (debug_handler.GenerateDebugInfo()) {
    GenerateIrDebugInfo(program, "optimized", debug_handler);
  }
  if (debug_handler.CheckIr()) {
    common::positions::FileSet ir_file_set;
    auto [ir_file, program_positions] =
        ::ir_serialization::PrintProgramToNewFile("ir.optimized.txt", program, ir_file_set);
    ir_issues::IssueTracker issue_tracker(&ir_file_set);
    ::ir_check::CheckProgram(program, program_positions, issue_tracker);
    if (!issue_tracker.issues().empty()) {
      *ctx->stderr() << "optimized IR program has issues:\n";
      issue_tracker.PrintIssues(common::issues::Format::kTerminal, ctx->stderr());
    }
  }
}

}  // namespace

std::variant<std::unique_ptr<ir::Program>, ErrorCode> Build(
    std::vector<std::filesystem::path>& paths, BuildOptions& options, DebugHandler& debug_handler,
    Context* ctx) {
  std::variant<std::unique_ptr<ir::Program>, ErrorCode> program_or_error =
      BuildIrProgram(paths, debug_handler, ctx);
  if (std::holds_alternative<ErrorCode>(program_or_error)) {
    return std::get<ErrorCode>(program_or_error);
  }
  auto ir_program = std::get<std::unique_ptr<ir::Program>>(std::move(program_or_error));

  if (options.optimize_ir_ext) {
    OptimizeIrExtProgram(ir_program.get(), debug_handler, ctx);
  }
  LowerIrExtProgram(ir_program.get(), debug_handler, ctx);
  if (options.optimize_ir) {
    OptimizeIrProgram(ir_program.get(), debug_handler, ctx);
  }

  return std::move(ir_program);
}

}  // namespace katara
}  // namespace cmd

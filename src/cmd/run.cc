//
//  run.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "run.h"

#include <iomanip>
#include <sstream>
#include <variant>

#include "src/cmd/build.h"
#include "src/common/memory/memory.h"
#include "src/ir/analyzers/interference_graph_builder.h"
#include "src/ir/analyzers/live_range_analyzer.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/processors/phi_resolver.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/x86_64/ir_translator/ir_translator.h"
#include "src/x86_64/machine_code/linker.h"

namespace cmd {
namespace {

std::string SubdirNameForFunc(ir::Func* func) {
  return "@" + std::to_string(func->number()) + "_" + func->name();
}

void GenerateX86_64DebugInfo(
    ir::Program* ir_program,
    std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraph>& interference_graphs,
    ir_to_x86_64_translator::TranslationResults& translation_results, DebugHandler& debug_handler) {
  debug_handler.WriteToDebugFile(translation_results.program->ToString(), /* subdir_name= */ "",
                                 "x86_64.asm.txt");

  for (auto& func : ir_program->funcs()) {
    std::string subdir_name = SubdirNameForFunc(func.get());

    const ir::func_num_t ir_func_num = func->number();
    const x86_64::func_num_t x86_64_func_num =
        translation_results.ir_to_x86_64_func_nums.at(ir_func_num);
    const x86_64::Func* x86_64_func =
        translation_results.program->DefinedFuncWithNumber(x86_64_func_num);
    const ir_info::InterferenceGraph& func_interference_graph =
        interference_graphs.at(func->number());
    const ir_info::InterferenceGraphColors& func_interference_graph_colors =
        translation_results.interference_graph_colors.at(func->number());

    debug_handler.WriteToDebugFile(x86_64_func->ToString(), subdir_name, "x86_64.asm.txt");
    debug_handler.WriteToDebugFile(
        func_interference_graph.ToGraph(&func_interference_graph_colors).ToDotFormat(), subdir_name,
        "x86_64.interference_graph.dot");
    debug_handler.WriteToDebugFile(func_interference_graph_colors.ToString(), subdir_name,
                                   "x86_64.colors.txt");
  }
}

std::unique_ptr<x86_64::Program> BuildX86_64Program(ir::Program* ir_program,
                                                    DebugHandler& debug_handler) {
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
  for (auto& func : ir_program->funcs()) {
    ir_processors::ResolvePhisInFunc(func.get());
  }

  ir_to_x86_64_translator::TranslationResults translation_results =
      ir_to_x86_64_translator::Translate(ir_program, live_ranges, interference_graphs,
                                         debug_handler.GenerateDebugInfo());
  if (debug_handler.GenerateDebugInfo()) {
    GenerateX86_64DebugInfo(ir_program, interference_graphs, translation_results, debug_handler);
  }
  return std::move(translation_results.program);
}

void* MallocJump(int size) {
  void* p = malloc(size);
  return p;
}

void FreeJump(void* ptr) { free(ptr); }

}  // namespace

ErrorCode Run(std::vector<std::filesystem::path>& paths, BuildOptions& options,
              DebugHandler& debug_handler, Context* ctx) {
  std::variant<std::unique_ptr<ir::Program>, ErrorCode> ir_program_or_error =
      Build(paths, options, debug_handler, ctx);
  if (std::holds_alternative<ErrorCode>(ir_program_or_error)) {
    return std::get<ErrorCode>(ir_program_or_error);
  }
  std::unique_ptr<ir::Program> ir_program =
      std::get<std::unique_ptr<ir::Program>>(std::move(ir_program_or_error));
  std::unique_ptr<x86_64::Program> x86_64_program =
      BuildX86_64Program(ir_program.get(), debug_handler);

  x86_64::Linker linker;
  linker.AddFuncAddr(x86_64_program->declared_funcs().at("malloc"), (uint8_t*)&MallocJump);
  linker.AddFuncAddr(x86_64_program->declared_funcs().at("free"), (uint8_t*)&FreeJump);

  common::Memory memory(common::Memory::kPageSize, common::Memory::kWrite);
  int64_t program_size = x86_64_program->Encode(linker, memory.data());
  linker.ApplyPatches();

  memory.ChangePermissions(common::Memory::kRead);
  if (debug_handler.GenerateDebugInfo()) {
    std::ostringstream buffer;
    for (int64_t j = 0; j < program_size; j++) {
      buffer << std::hex << std::setfill('0') << std::setw(2) << (unsigned short)memory.data()[j]
             << " ";
      if (j % 8 == 7 && j != program_size - 1) {
        buffer << "\n";
      }
    }
    debug_handler.WriteToDebugFile(buffer.str(), /* subdir_name= */ "", "x86_64.hex.txt");
  }

  memory.ChangePermissions(common::Memory::kExecute);
  x86_64::Func* x86_64_main_func = x86_64_program->DefinedFuncWithName("main");
  int (*main_func)(void) = (int (*)(void))(linker.func_addrs().at(x86_64_main_func->func_num()));
  return ErrorCode(main_func());
}

}  // namespace cmd

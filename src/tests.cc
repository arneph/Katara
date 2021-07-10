//
//  tests.cc
//  Katara
//
//  Created by Arne Philipeit on 06/03/21.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

#include "src/ir/processors/live_range_analyzer.h"
#include "src/ir/processors/parser.h"
#include "src/ir/processors/phi_resolver.h"
#include "src/ir/processors/register_allocator.h"
#include "src/ir/processors/scanner.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/lang/processors/docs/file_doc.h"
#include "src/lang/processors/docs/package_doc.h"
#include "src/lang/processors/ir_builder/ir_builder.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/processors/packages/package_manager.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/ast/ast_util.h"
#include "src/lang/representation/positions/positions.h"
#include "src/lang/representation/types/info_util.h"

void to_file(std::string text, std::filesystem::path out_file) {
  std::ofstream out_stream(out_file, std::ios::out);

  out_stream << text;
}

void run_ir_test(std::filesystem::path test_dir) {
  std::string test_name = test_dir.filename();
  std::cout << "testing " + test_name << "\n";

  std::filesystem::path in_file = test_dir.string() + "/" + test_name + ".ir";
  std::filesystem::path out_file_base = test_dir.string() + "/" + test_name;

  if (!std::filesystem::exists(in_file)) {
    std::cout << "test file not found\n";
    return;
  }

  std::ifstream in_stream(in_file, std::ios::in);
  ir_proc::Scanner scanner(in_stream);
  std::unique_ptr<ir::Program> prog = ir_proc::Parser::Parse(scanner);

  std::cout << prog->ToString() << "\n";

  for (auto& func : prog->funcs()) {
    common::Graph cfg = func->ToControlFlowGraph();
    common::Graph dom_tree = func->ToDominatorTree();

    to_file(cfg.ToDotFormat(),
            out_file_base.string() + ".init.@" + std::to_string(func->number()) + ".cfg.dot");
    to_file(dom_tree.ToDotFormat(),
            out_file_base.string() + ".init.@" + std::to_string(func->number()) + ".dom.dot");
  }

  std::unordered_map<ir::Func*, ir_info::FuncLiveRangeInfo> live_range_infos;
  std::unordered_map<ir::Func*, ir_info::InterferenceGraph> interference_graphs;

  for (auto& func : prog->funcs()) {
    ir_proc::LiveRangeAnalyzer live_range_analyzer(func.get());

    ir_info::FuncLiveRangeInfo& func_live_range_info = live_range_analyzer.func_info();
    ir_info::InterferenceGraph& interference_graph = live_range_analyzer.interference_graph();

    live_range_infos.insert({func.get(), func_live_range_info});
    interference_graphs.insert({func.get(), interference_graph});

    to_file(
        func_live_range_info.ToString(),
        out_file_base.string() + ".@" + std::to_string(func->number()) + ".live_range_info.txt");
  }

  x86_64_ir_translator::IRTranslator translator(prog.get(), interference_graphs);
  translator.PrepareInterferenceGraphs();

  for (auto& func : prog->funcs()) {
    ir_info::InterferenceGraph& interference_graph = interference_graphs.at(func.get());

    ir_proc::RegisterAllocator register_allocator(func.get(), interference_graph);
    register_allocator.AllocateRegisters();

    to_file(
        interference_graph.ToString(),
        out_file_base.string() + ".@" + std::to_string(func->number()) + ".interference_graph.txt");
    to_file(
        interference_graph.ToGraph().ToDotFormat(),
        out_file_base.string() + ".@" + std::to_string(func->number()) + ".interference_graph.dot");

    ir_proc::PhiResolver phi_resolver(func.get());
    phi_resolver.ResolvePhis();

    common::Graph cfg = func->ToControlFlowGraph();
    common::Graph dom_tree = func->ToDominatorTree();

    to_file(cfg.ToDotFormat(),
            out_file_base.string() + ".final.@" + std::to_string(func->number()) + ".cfg.dot");
    to_file(dom_tree.ToDotFormat(),
            out_file_base.string() + ".final.@" + std::to_string(func->number()) + ".dom.dot");
  }

  translator.TranslateProgram();

  x86_64::Program* x86_64_program = translator.x86_64_program();

  to_file(x86_64_program->ToString(), out_file_base.string() + ".x86_64.txt");
}

void test_ir() {
  std::filesystem::path ir_tests = "/Users/arne/Documents/Xcode/Katara/tests/ir";

  std::cout << "running ir-tests\n";

  for (auto entry : std::filesystem::directory_iterator(ir_tests)) {
    if (!entry.is_directory()) {
      continue;
    }
    run_ir_test(entry.path());
  }

  std::cout << "completed ir-tests\n";
}

int main() {
  test_ir();

  return 0;
}

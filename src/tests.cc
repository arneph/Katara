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
#include <sstream>

#include "src/ir/analyzers/interference_graph_builder.h"
#include "src/ir/analyzers/live_range_analyzer.h"
#include "src/ir/info/func_live_ranges.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/processors/phi_resolver.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/program.h"
#include "src/ir/serialization/parse.h"
#include "src/ir/serialization/print.h"
#include "src/ir/serialization/scanner.h"
#include "src/x86_64/ir_translator/ir_translator.h"
#include "src/x86_64/program.h"

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

  std::ifstream in_fstream(in_file, std::ios::in);
  std::stringstream in_sstream;
  in_sstream << in_fstream.rdbuf();
  std::unique_ptr<ir::Program> ir_program = ir_serialization::ParseProgramOrDie(in_sstream.str());

  std::cout << ir_serialization::PrintProgram(ir_program.get());
  std::cout << "\n";

  for (auto& func : ir_program->funcs()) {
    common::graph::Graph cfg = func->ToControlFlowGraph();
    common::graph::Graph dom_tree = func->ToDominatorTree();

    to_file(cfg.ToDotFormat(),
            out_file_base.string() + ".init.@" + std::to_string(func->number()) + ".cfg.dot");
    to_file(dom_tree.ToDotFormat(),
            out_file_base.string() + ".init.@" + std::to_string(func->number()) + ".dom.dot");
  }

  std::unordered_map<ir::func_num_t, const ir_info::FuncLiveRanges> live_ranges;
  std::unordered_map<ir::func_num_t, const ir_info::InterferenceGraph> interference_graphs;

  for (auto& func : ir_program->funcs()) {
    const ir_info::FuncLiveRanges func_live_ranges =
        ir_analyzers::FindLiveRangesForFunc(func.get());
    const ir_info::InterferenceGraph func_interference_graph =
        ir_analyzers::BuildInterferenceGraphForFunc(func.get(), func_live_ranges);

    live_ranges.insert({func->number(), func_live_ranges});
    interference_graphs.insert({func->number(), func_interference_graph});

    to_file(
        func_live_ranges.ToString(),
        out_file_base.string() + ".@" + std::to_string(func->number()) + ".live_range_info.txt");
    to_file(
        func_interference_graph.ToString(),
        out_file_base.string() + ".@" + std::to_string(func->number()) + ".interference_graph.txt");
    to_file(
        func_interference_graph.ToGraph().ToDotFormat(),
        out_file_base.string() + ".@" + std::to_string(func->number()) + ".interference_graph.dot");
  }

  for (auto& func : ir_program->funcs()) {
    ir_processors::ResolvePhisInFunc(func.get());
    common::graph::Graph cfg = func->ToControlFlowGraph();
    common::graph::Graph dom_tree = func->ToDominatorTree();

    to_file(cfg.ToDotFormat(),
            out_file_base.string() + ".final.@" + std::to_string(func->number()) + ".cfg.dot");
    to_file(dom_tree.ToDotFormat(),
            out_file_base.string() + ".final.@" + std::to_string(func->number()) + ".dom.dot");
  }

  ir_to_x86_64_translator::TranslationResults translation_results =
      ir_to_x86_64_translator::Translate(ir_program.get(), live_ranges, interference_graphs);

  to_file(translation_results.program->ToString(), out_file_base.string() + ".x86_64.txt");
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

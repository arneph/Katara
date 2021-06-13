//
//  tests.cc
//  Katara
//
//  Created by Arne Philipeit on 06/03/21.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
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
#include "src/lang/processors/packages/packages.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/ast/ast_util.h"
#include "src/lang/representation/positions/positions.h"
#include "src/lang/representation/types/info_util.h"
#include "src/x86_64/block.h"
#include "src/x86_64/func.h"
#include "src/x86_64/instr.h"
#include "src/x86_64/instrs/al_instrs.h"
#include "src/x86_64/instrs/cf_instrs.h"
#include "src/x86_64/instrs/data_instrs.h"
#include "src/x86_64/ir_translator/ir_translator.h"
#include "src/x86_64/mc/linker.h"
#include "src/x86_64/ops.h"
#include "src/x86_64/prog.h"

void to_file(std::string text, std::filesystem::path out_file) {
  std::ofstream out_stream(out_file, std::ios::out);

  out_stream << text;
}

void run_lang_test(std::filesystem::path test_dir) {
  std::string test_name = test_dir.filename();
  std::cout << "testing " + test_name << "\n";
  std::filesystem::path docs_dir = test_dir / "docs";
  std::filesystem::path debug_dir = test_dir / "debug";
  std::filesystem::create_directory(docs_dir);
  std::filesystem::create_directory(debug_dir);

  lang::packages::PackageManager pkg_manager("/Users/arne/Documents/Xcode/Katara/stdlib");
  lang::packages::Package* test_pkg = pkg_manager.LoadPackage(test_dir);

  for (auto pkg : pkg_manager.Packages()) {
    for (auto& issue : pkg->issue_tracker().issues()) {
      switch (issue.severity()) {
        case lang::issues::Severity::kWarning:
          std::cout << "\033[93;1m";
          std::cout << "Warning:";
          std::cout << "\033[0;0m";
          std::cout << " ";
          break;
        case lang::issues::Severity::kError:
        case lang::issues::Severity::kFatal:
          std::cout << "\033[91;1m";
          std::cout << "Error:";
          std::cout << "\033[0;0m";
          std::cout << " ";
          break;
      }
      std::cout << issue.message() << " [" << issue.kind_id() << "]\n";
      for (lang::pos::pos_t pos : issue.positions()) {
        lang::pos::Position position = pkg_manager.file_set()->PositionFor(pos);
        std::string line = pkg_manager.file_set()->FileAt(pos)->LineFor(pos);
        size_t whitespace = 0;
        for (; whitespace < line.length(); whitespace++) {
          if (line.at(whitespace) != ' ' && line.at(whitespace) != '\t') {
            break;
          }
        }
        std::cout << "  " << position.ToString() << ": ";
        std::cout << line.substr(whitespace);
        size_t pointer = 4 + position.ToString().size() + position.column_ - whitespace;
        for (size_t i = 0; i < pointer; i++) {
          std::cout << " ";
        }
        std::cout << "^\n";
      }
    }
  }
  for (auto [name, ast_file] : test_pkg->ast_package()->files()) {
    common::Graph ast_graph = lang::ast::NodeToTree(pkg_manager.file_set(), ast_file);

    to_file(ast_graph.ToDotFormat(), debug_dir / (test_pkg->name() + ".ast.dot"));
  }

  std::string type_info = lang::types::InfoToText(pkg_manager.file_set(), pkg_manager.type_info());
  to_file(type_info, debug_dir / (test_pkg->name() + ".types.txt"));

  lang::docs::PackageDoc pkg_doc = lang::docs::GenerateDocumentationForPackage(
      test_pkg, pkg_manager.file_set(), pkg_manager.type_info());
  to_file(pkg_doc.html, docs_dir / (pkg_doc.name + ".html"));
  for (auto file_doc : pkg_doc.docs) {
    to_file(file_doc.html, docs_dir / (file_doc.name + ".html"));
  }

  std::unique_ptr<ir::Program> program =
      lang::ir_builder::IRBuilder::TranslateProgram(test_pkg, pkg_manager.type_info());
  if (program) {
    to_file(program->ToString(), debug_dir / (test_pkg->name() + ".ir.txt"));

    for (auto& func : program->funcs()) {
      std::string file_name =
          test_pkg->name() + "_@" + std::to_string(func->number()) + "_" + func->name();
      common::Graph func_cfg = func->ToControlFlowGraph();
      common::Graph func_dom = func->ToDominatorTree();

      to_file(func_cfg.ToDotFormat(), debug_dir / (file_name + ".cfg.dot"));
      to_file(func_dom.ToDotFormat(), debug_dir / (file_name + ".dom.dot"));
    }
  }
}

void test_lang() {
  std::filesystem::path lang_tests = "/Users/arne/Documents/Xcode/Katara/tests/lang";

  std::cout << "running lang-tests\n";
  std::vector<std::filesystem::directory_entry> entries;
  for (auto entry : std::filesystem::directory_iterator(lang_tests)) {
    entries.push_back(entry);
  }
  std::sort(entries.begin(), entries.end());
  for (auto entry : entries) {
    if (!entry.is_directory()) {
      continue;
    }
    run_lang_test(entry.path());
  }

  std::cout << "completed lang-tests\n";
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

  x86_64_ir_translator::IRTranslator translator(prog.get(), live_range_infos, interference_graphs);
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

  x86_64::Prog* x86_64_program = translator.x86_64_program();

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

long AddInts(long a, long b) { return a + b; }

void PrintInt(long value) { printf("%ld\n", value); }

void test_x86_64() {
  std::cout << "running x86-tests\n";

  uint8_t* add_ints_addr = (uint8_t*)&AddInts;
  uint8_t* print_int_addr = (uint8_t*)&PrintInt;

  x86_64::Linker* linker = new x86_64::Linker();
  linker->AddFuncAddr(1234, add_ints_addr);
  linker->AddFuncAddr(1235, print_int_addr);

  const char* str = "Hello world!\n";
  x86_64::Imm str_c((int64_t(str)));

  x86_64::ProgBuilder prog_builder;
  x86_64::FuncBuilder main_func_builder = prog_builder.AddFunc("main");

  // Prolog:
  {
    x86_64::BlockBuilder prolog_block_builder = main_func_builder.AddBlock();
    prolog_block_builder.AddInstr(new x86_64::Push(x86_64::rbp));
    prolog_block_builder.AddInstr(new x86_64::Mov(x86_64::rbp, x86_64::rsp));
  }

  // Fibonacci numbers:
  {
    x86_64::BlockBuilder start_block_builder = main_func_builder.AddBlock();
    start_block_builder.AddInstr(new x86_64::Mov(x86_64::r15b, x86_64::Imm(int8_t{10})));
    start_block_builder.AddInstr(new x86_64::Mov(x86_64::r12, x86_64::Imm(int64_t{1})));
    start_block_builder.AddInstr(new x86_64::Mov(x86_64::r13, x86_64::Imm(int64_t{1})));
    start_block_builder.AddInstr(new x86_64::Mov(x86_64::rdi, x86_64::r12));
    start_block_builder.AddInstr(new x86_64::Call(x86_64::FuncRef(1235)));
  }
  {
    x86_64::BlockBuilder loop_block_builder = main_func_builder.AddBlock();
    loop_block_builder.AddInstr(new x86_64::Mov(x86_64::rdi, x86_64::r12));
    loop_block_builder.AddInstr(new x86_64::Call(x86_64::FuncRef(1235)));
    loop_block_builder.AddInstr(new x86_64::Mov(x86_64::r14, x86_64::r12));
    loop_block_builder.AddInstr(new x86_64::Add(x86_64::r14, x86_64::r13));
    loop_block_builder.AddInstr(new x86_64::Mov(x86_64::r13, x86_64::r12));
    loop_block_builder.AddInstr(new x86_64::Mov(x86_64::r12, x86_64::r14));
    loop_block_builder.AddInstr(new x86_64::Sub(x86_64::r15b, x86_64::Imm(int8_t{1})));
    loop_block_builder.AddInstr(
        new x86_64::Jcc(x86_64::InstrCond::kAbove, loop_block_builder.block()->GetBlockRef()));
  }

  // Hello world (Syscall test):
  {
    x86_64::BlockBuilder hello_block_builder = main_func_builder.AddBlock();
    hello_block_builder.AddInstr(
        new x86_64::Mov(x86_64::rax, x86_64::Imm(int64_t{0x2000004})));                   // write
    hello_block_builder.AddInstr(new x86_64::Mov(x86_64::rdi, x86_64::Imm(int32_t{1})));  // stdout
    hello_block_builder.AddInstr(new x86_64::Mov(x86_64::rsi, str_c));  // const char
    hello_block_builder.AddInstr(new x86_64::Mov(x86_64::rdx, x86_64::Imm(int32_t{13})));  // size
    hello_block_builder.AddInstr(new x86_64::Syscall());
    // hello_block_builder.AddInstr(std::make_unique<x86_64::Jmp>(hello_block_builder.block()->GetBlockRef()));
  }

  // Other tests:
  {
    x86_64::BlockBuilder test_block_builder = main_func_builder.AddBlock();
    test_block_builder.AddInstr(new x86_64::Mov(x86_64::rdi, x86_64::Imm(int32_t{1})));
    test_block_builder.AddInstr(new x86_64::Mov(x86_64::rsi, x86_64::Imm(int32_t{2})));
    test_block_builder.AddInstr(new x86_64::Call(x86_64::FuncRef(1234)));
    test_block_builder.AddInstr(new x86_64::Mov(x86_64::rdi, x86_64::rax));
    test_block_builder.AddInstr(new x86_64::Sub(x86_64::rdi, x86_64::rax));
    test_block_builder.AddInstr(new x86_64::Add(x86_64::rdi, x86_64::rax));
    test_block_builder.AddInstr(new x86_64::Add(x86_64::rdi, x86_64::Imm(int8_t{17})));
    test_block_builder.AddInstr(new x86_64::Sub(x86_64::rdi, x86_64::Imm(int8_t{6})));
    test_block_builder.AddInstr(new x86_64::Call(x86_64::FuncRef(1235)));
    test_block_builder.AddInstr(new x86_64::Mov(x86_64::rdi, x86_64::Imm(int32_t{1233})));
    test_block_builder.AddInstr(new x86_64::Sub(x86_64::rdi, x86_64::Imm(int32_t{-1})));
    test_block_builder.AddInstr(new x86_64::Call(x86_64::FuncRef(1235)));
  }

  // Epilog:
  {
    x86_64::BlockBuilder epilog_block_builder = main_func_builder.AddBlock();
    epilog_block_builder.AddInstr(new x86_64::Mov(x86_64::rsp, x86_64::rbp));
    epilog_block_builder.AddInstr(new x86_64::Pop(x86_64::rbp));
    epilog_block_builder.AddInstr(new x86_64::Ret());
  }

  x86_64::Prog* prog = prog_builder.prog();

  std::cout << prog->ToString() << std::endl << std::endl;

  uint8_t* base =
      (uint8_t*)mmap(NULL, 1 << 12, PROT_EXEC | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  common::data code(base, 1 << 12);

  int64_t size = prog->Encode(linker, code);
  linker->ApplyPatches();

  for (int64_t j = 0; j < size; j++) {
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short)code[j] << " ";
  }
  std::cout << std::endl;

  void (*func)(void) = (void (*)(void))base;
  func();

  std::cout << "completed x86-tests\n";
}

int main() {
  test_lang();
  // test_ir();
  // test_x86_64();

  return 0;
}

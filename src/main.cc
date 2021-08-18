//
//  main.cc
//  Katara
//
//  Created by Arne Philipeit on 11/22/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include <sys/mman.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "src/ir/analyzers/interference_graph_builder.h"
#include "src/ir/analyzers/live_range_analyzer.h"
#include "src/ir/info/func_live_ranges.h"
#include "src/ir/info/interference_graph.h"
#include "src/ir/interpreter/interpreter.h"
#include "src/ir/processors/phi_resolver.h"
#include "src/ir/representation/program.h"
#include "src/lang/processors/docs/file_doc.h"
#include "src/lang/processors/docs/package_doc.h"
#include "src/lang/processors/ir_builder/ir_builder.h"
#include "src/lang/processors/ir_lowerers/shared_pointer_lowerer.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/processors/packages/package_manager.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/ast/ast_util.h"
#include "src/lang/representation/positions/positions.h"
#include "src/lang/representation/types/info_util.h"
#include "src/x86_64/ir_translator/ir_translator.h"

constexpr std::string_view kVersion = "0.1";
constexpr std::string_view kStdLibPath = "/Users/arne/Documents/Xcode/Katara/stdlib";

void printHelp(std::ostream& out) {
  out << "Katara is a tool to work with Katara source code.\n"
         "\n"
         "Usage:\n"
         "\n"
         "\tkatara <command> [arguments]\n"
         "\n"
         "The commands are:\n"
         "\n"
         "\tbuild\tbuild Katara packages\n"
         "\tdoc\tgenerate documentation for Katara packages\n"
         "\thelp\tprint this documentation\n"
         "\trun\trun Katara programs\n"
         "\tversion\tprint Katara version\n"
         "\n";
}

void printVersion(std::ostream& out) { out << "Katara version " << kVersion << "\n"; }

void to_file(std::string text, std::filesystem::path out_file) {
  std::ofstream out_stream(out_file, std::ios::out);

  out_stream << text;
}

struct LoadResult {
  std::unique_ptr<lang::packages::PackageManager> pkg_manager;
  std::vector<lang::packages::Package*> arg_pkgs;
  bool generate_debug_info;
  int exit_code;
};

LoadResult load(const std::vector<std::string> args, std::ostream& err) {
  enum class ArgsKind {
    kNone,
    kMainPackageDirectory,
    kMainPackageFiles,
    kPackagePaths,
  };
  ArgsKind args_kind = ArgsKind::kNone;
  std::vector<std::string> args_without_flags;
  bool generate_debug_info = false;
  for (std::string arg : args) {
    if (arg == "-d" || arg == "--debug") {
      generate_debug_info = true;
      continue;
    }
    std::filesystem::path path(arg);
    path = std::filesystem::absolute(path);
    if (path.extension() == ".kat") {
      if (args_kind != ArgsKind::kNone && args_kind != ArgsKind::kMainPackageFiles) {
        err << "source file arguments can not be mixed with package path arguments\n";
        return LoadResult{.pkg_manager = nullptr,
                          .arg_pkgs = {},
                          .generate_debug_info = generate_debug_info,
                          .exit_code = 1};
      }
      args_kind = ArgsKind::kMainPackageFiles;
      arg = path;
      args_without_flags.push_back(arg);
      continue;
    }

    if (std::filesystem::is_directory(path)) {
      if (args_kind != ArgsKind::kNone) {
        err << "can only handle one main package path argument\n";
        return LoadResult{.pkg_manager = nullptr,
                          .arg_pkgs = {},
                          .generate_debug_info = generate_debug_info,
                          .exit_code = 2};
      }
      args_kind = ArgsKind::kMainPackageDirectory;
      arg = path;
    } else {
      if (args_kind != ArgsKind::kNone && args_kind != ArgsKind::kPackagePaths) {
        err << "source file arguments can not be mixed with package path arguments\n";
        return LoadResult{.pkg_manager = nullptr,
                          .arg_pkgs = {},
                          .generate_debug_info = generate_debug_info,
                          .exit_code = 3};
      }
      args_kind = ArgsKind::kPackagePaths;
    }
    args_without_flags.push_back(arg);
  }

  auto pkg_manager = std::make_unique<lang::packages::PackageManager>(
      std::string{kStdLibPath}, std::filesystem::current_path());
  lang::packages::Package* main_pkg = nullptr;
  std::vector<lang::packages::Package*> arg_pkgs;
  switch (args_kind) {
    case ArgsKind::kNone:
      main_pkg = pkg_manager->LoadMainPackage(std::filesystem::current_path());
      break;
    case ArgsKind::kMainPackageDirectory:
      main_pkg = pkg_manager->LoadMainPackage(args_without_flags.front());
      break;
    case ArgsKind::kMainPackageFiles:
      main_pkg = pkg_manager->LoadMainPackage(args_without_flags);
      break;
    case ArgsKind::kPackagePaths:
      for (std::string& pkg_path : args_without_flags) {
        lang::packages::Package* pkg = pkg_manager->LoadPackage(pkg_path);
        if (pkg != nullptr) {
          arg_pkgs.push_back(pkg);
        }
      }
      break;
  }
  if (main_pkg != nullptr) {
    arg_pkgs.push_back(main_pkg);
  }

  bool contains_issues = !pkg_manager->issue_tracker()->issues().empty();
  pkg_manager->issue_tracker()->PrintIssues(lang::issues::IssueTracker::PrintFormat::kTerminal,
                                            err);
  for (auto pkg : pkg_manager->Packages()) {
    pkg->issue_tracker().PrintIssues(lang::issues::IssueTracker::PrintFormat::kTerminal, err);
    if (!pkg->issue_tracker().issues().empty()) {
      contains_issues = true;
    }
  }
  if (generate_debug_info) {
    for (lang::packages::Package* pkg : arg_pkgs) {
      std::filesystem::path pkg_dir{pkg->dir()};
      std::filesystem::path debug_dir = pkg_dir / "debug";
      std::filesystem::create_directory(debug_dir);

      for (auto [name, ast_file] : main_pkg->ast_package()->files()) {
        common::Graph ast_graph = lang::ast::NodeToTree(pkg_manager->file_set(), ast_file);

        to_file(ast_graph.ToDotFormat(), debug_dir / (name + ".ast.dot"));
      }

      std::string type_info =
          lang::types::InfoToText(pkg_manager->file_set(), pkg_manager->type_info());
      to_file(type_info, debug_dir / (main_pkg->name() + ".types.txt"));
    }
  }
  if (contains_issues) {
    return LoadResult{
        .pkg_manager = std::move(pkg_manager),
        .arg_pkgs = arg_pkgs,
        .generate_debug_info = generate_debug_info,
        .exit_code = 4,
    };
  }

  return LoadResult{
      .pkg_manager = std::move(pkg_manager),
      .arg_pkgs = arg_pkgs,
      .generate_debug_info = generate_debug_info,
      .exit_code = 0,
  };
}

int doc(const std::vector<std::string> args, std::ostream& err) {
  auto [pkg_manager, arg_pkgs, generate_debug_info, exit_code] = load(args, err);
  if (exit_code) {
    return exit_code;
  }

  for (lang::packages::Package* pkg : arg_pkgs) {
    std::filesystem::path pkg_dir{pkg->dir()};
    std::filesystem::path docs_dir = pkg_dir / "doc";
    std::filesystem::create_directory(docs_dir);

    lang::docs::PackageDoc pkg_doc = lang::docs::GenerateDocumentationForPackage(
        pkg, pkg_manager->file_set(), pkg_manager->type_info());

    to_file(pkg_doc.html, docs_dir / (pkg_doc.name + ".html"));
    for (auto file_doc : pkg_doc.docs) {
      to_file(file_doc.html, docs_dir / (file_doc.name + ".html"));
    }
  }
  return 0;
}

struct BuildResult {
  std::unique_ptr<ir::Program> ir_program;
  std::unique_ptr<x86_64::Program> x86_64_program;
  int exit_code;
};

BuildResult build(const std::vector<std::string> args, std::ostream& err) {
  auto [pkg_manager, arg_pkgs, generate_debug_info, exit_code] = load(args, err);
  if (exit_code) {
    return BuildResult{
        .ir_program = nullptr,
        .x86_64_program = nullptr,
        .exit_code = exit_code,
    };
  }

  lang::packages::Package* main_pkg = pkg_manager->GetMainPackage();
  if (main_pkg == nullptr) {
    // TODO: support translating non-main packages to IR
    return BuildResult{
        .ir_program = nullptr,
        .x86_64_program = nullptr,
        .exit_code = 5,
    };
  }

  std::filesystem::path debug_dir = (main_pkg != nullptr)
                                        ? std::filesystem::path(main_pkg->dir()) / "debug"
                                        : std::filesystem::current_path();
  auto ir_program_debug_info_generator =
      [main_pkg, debug_dir](
          ir::Program* program, std::string iter,
          std::unordered_map<ir::func_num_t, const ir_info::FuncLiveRanges>* live_ranges = nullptr,
          std::unordered_map<ir::func_num_t,
                             const ir_info::InterferenceGraph>* interference_graphs = nullptr) {
        to_file(program->ToString(), debug_dir / ("ir." + iter + ".txt"));

        for (auto& func : program->funcs()) {
          ir::func_num_t func_num = func->number();
          std::string file_name =
              main_pkg->name() + "_@" + std::to_string(func_num) + "_" + func->name() + "." + iter;
          common::Graph func_cfg = func->ToControlFlowGraph();
          common::Graph func_dom = func->ToDominatorTree();

          to_file(func_cfg.ToDotFormat(), debug_dir / (file_name + ".cfg.dot"));
          to_file(func_dom.ToDotFormat(), debug_dir / (file_name + ".dom.dot"));
          if (live_ranges != nullptr) {
            const ir_info::FuncLiveRanges& func_live_ranges = live_ranges->at(func->number());

            to_file(func_live_ranges.ToString(), debug_dir / (file_name + ".live_range_info.txt"));
          }
          if (interference_graphs != nullptr) {
            const ir_info::InterferenceGraph& func_interference_graph =
                interference_graphs->at(func->number());

            to_file(func_interference_graph.ToString(),
                    debug_dir / (file_name + ".interference_graph.txt"));
            to_file(func_interference_graph.ToGraph().ToDotFormat(),
                    debug_dir / (file_name + ".interference_graph.dot"));
          }
        }
      };

  std::unique_ptr<ir::Program> ir_program =
      lang::ir_builder::IRBuilder::TranslateProgram(main_pkg, pkg_manager->type_info());
  if (ir_program == nullptr) {
    return BuildResult{
        .ir_program = nullptr,
        .x86_64_program = nullptr,
        .exit_code = 6,
    };
  }
  if (generate_debug_info) {
    ir_program_debug_info_generator(ir_program.get(), "init");
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
  if (generate_debug_info) {
    ir_program_debug_info_generator(ir_program.get(), "lowered", &live_ranges,
                                    &interference_graphs);
  }

  for (auto& func : ir_program->funcs()) {
    ir_processors::ResolvePhisInFunc(func.get());
  }

  ir_to_x86_64_translator::TranslationResults translation_results =
      ir_to_x86_64_translator::Translate(ir_program.get(), live_ranges, interference_graphs,
                                         generate_debug_info);

  if (generate_debug_info) {
    to_file(translation_results.program->ToString(), debug_dir / "x86_64.txt");
    for (auto& func : ir_program->funcs()) {
      ir::func_num_t func_num = func->number();
      std::string file_name =
          main_pkg->name() + "_@" + std::to_string(func_num) + "_" + func->name() + ".x86_64";

      const ir_info::InterferenceGraph& func_interference_graph = interference_graphs.at(func_num);
      const ir_info::InterferenceGraphColors& func_interference_graph_colors =
          translation_results.interference_graph_colors.at(func_num);

      to_file(func_interference_graph.ToGraph(&func_interference_graph_colors).ToDotFormat(),
              debug_dir / (file_name + ".interference_graph.dot"));
      to_file(func_interference_graph_colors.ToString(), debug_dir / (file_name + ".colors.txt"));
    }
  }

  return BuildResult{
      .ir_program = std::move(ir_program),
      .x86_64_program = std::move(translation_results.program),
      .exit_code = 0,
  };
}

int run(const std::vector<std::string> args, std::istream& in, std::ostream& out,
        std::ostream& err) {
  auto [ir_program, x86_64_program, exit_code] = build(args, err);
  if (exit_code) {
    return exit_code;
  }

  // ir_interpreter::Interpreter interpreter(ir_program.get());
  // interpreter.run();

  // return static_cast<int>(interpreter.exit_code());

  x86_64::Linker linker;
  linker.AddFuncAddr(x86_64_program->declared_funcs().at("malloc"), (uint8_t*)&malloc);
  linker.AddFuncAddr(x86_64_program->declared_funcs().at("free"), (uint8_t*)&free);

  int64_t page_size = 1 << 12;
  uint8_t* base =
      (uint8_t*)mmap(NULL, page_size, PROT_EXEC | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  common::DataView code(base, page_size);

  int64_t program_size = x86_64_program->Encode(linker, code);
  linker.ApplyPatches();

  std::cout << "BEGIN machine code\n";
  for (int64_t j = 0; j < program_size; j++) {
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned short)code[j] << " ";
    if (j % 8 == 7 && j != program_size - 1) {
      std::cout << "\n";
    }
  }
  std::cout << "END machine code\n";

  x86_64::Func* x86_64_main_func = x86_64_program->DefinedFuncWithName("main");
  int (*main_func)(void) = (int (*)(void))(linker.func_addrs().at(x86_64_main_func->func_num()));
  return main_func();
}

int main(int argc, char* argv[]) {
  if (argc <= 1) {
    printHelp(std::cerr);
    return 0;
  }
  std::string command(argv[1]);
  std::vector<std::string> args(argv + 1, argv + argc);

  if (command == "build") {
    return build(args, std::cerr).exit_code;
  } else if (command == "doc") {
    return doc(args, std::cerr);
  } else if (command == "run") {
    return run(args, std::cin, std::cout, std::cerr);
  } else if (command == "version") {
    printVersion(std::cerr);
    return 0;
  } else {
    printHelp(std::cerr);
    return 0;
  }
}

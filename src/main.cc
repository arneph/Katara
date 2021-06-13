//
//  main.cc
//  Katara
//
//  Created by Arne Philipeit on 11/22/19.
//  Copyright © 2019 Arne Philipeit. All rights reserved.
//

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "src/lang/processors/ir_builder/ir_builder.h"
#include "src/lang/processors/packages/packages.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/ast/ast_util.h"
#include "src/lang/representation/positions/positions.h"
#include "src/lang/representation/types/info_util.h"
#include "src/ir/representation/program.h"
#include "src/ir/interpreter/interpreter.h"

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
         "\thelp\tprint this documentation\n"
         "\trun\trun Katara programs\n"
         "\tversion\tprint Katara version\n"
         "\n";
}

void printVersion(std::ostream& out) { out << "Katara version " << kVersion << "\n"; }

void printIssues(lang::pos::FileSet* file_set, const std::vector<lang::issues::Issue>& issues,
                 std::ostream& out) {
  for (auto& issue : issues) {
    switch (issue.severity()) {
      case lang::issues::Severity::kWarning:
        out << "\033[93;1m"
               "Warning:"
               "\033[0;0m"
               " ";
        break;
      case lang::issues::Severity::kError:
      case lang::issues::Severity::kFatal:
        out << "\033[91;1m"
               "Error:"
               "\033[0;0m"
               " ";
        break;
    }
    out << issue.message() << " [" << issue.kind_id() << "]\n";
    for (lang::pos::pos_t pos : issue.positions()) {
      lang::pos::Position position = file_set->PositionFor(pos);
      std::string line = file_set->FileAt(pos)->LineFor(pos);
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
        out << " ";
      }
      out << "^\n";
    }
  }
}

void to_file(std::string text, std::filesystem::path out_file) {
  std::ofstream out_stream(out_file, std::ios::out);

  out_stream << text;
}

int run(const std::vector<const std::string> args, std::istream& in, std::ostream& out,
        std::ostream& err) {
  std::filesystem::path package_dir = std::filesystem::current_path();
  bool generate_debug_info = false;
  for (auto& arg : args) {
    if (arg == "-d" || arg == "--debug") {
      generate_debug_info = true;
      continue;
    }
    package_dir = arg;
  }
  std::filesystem::path debug_dir = package_dir / "debug";
  if (generate_debug_info) {
    std::filesystem::create_directory(debug_dir);
  }

  lang::packages::PackageManager pkg_manager(std::string{kStdLibPath});
  lang::packages::Package* main_pkg = pkg_manager.LoadPackage(package_dir);

  bool contains_issues = false;
  for (auto pkg : pkg_manager.Packages()) {
    printIssues(pkg_manager.file_set(), pkg->issue_tracker().issues(), err);
    if (!pkg->issue_tracker().issues().empty()) {
      contains_issues = true;
    }
  }
  if (generate_debug_info) {
    for (auto [name, ast_file] : main_pkg->ast_package()->files()) {
      common::Graph ast_graph = lang::ast::NodeToTree(pkg_manager.file_set(), ast_file);

      to_file(ast_graph.ToDotFormat(), debug_dir / (main_pkg->name() + ".ast.dot"));
    }

    std::string type_info =
        lang::types::InfoToText(pkg_manager.file_set(), pkg_manager.type_info());
    to_file(type_info, debug_dir / (main_pkg->name() + ".types.txt"));
  }
  if (contains_issues) {
    return 1;
  }

  std::unique_ptr<ir::Program> program =
      lang::ir_builder::IRBuilder::TranslateProgram(main_pkg, pkg_manager.type_info());
  if (generate_debug_info && program) {
    to_file(program->ToString(), debug_dir / (main_pkg->name() + ".ir.txt"));

    for (auto& func : program->funcs()) {
      std::string file_name =
          main_pkg->name() + "_@" + std::to_string(func->number()) + "_" + func->name();
      common::Graph func_cfg = func->ToControlFlowGraph();
      common::Graph func_dom = func->ToDominatorTree();

      to_file(func_cfg.ToDotFormat(), debug_dir / (file_name + ".cfg.dot"));
      to_file(func_dom.ToDotFormat(), debug_dir / (file_name + ".dom.dot"));
    }
  }

  ir_interpreter::Interpreter interpreter(program.get());
  interpreter.run();
  
  return interpreter.exit_code();
}

int main(int argc, char* argv[]) {
  if (argc <= 1) {
    printHelp(std::cerr);
    return 0;
  }
  std::string command(argv[1]);
  if (command == "run") {
    std::vector<const std::string> args;
    args.reserve(argc - 2);
    for (int i = 2; i < argc; i++) {
      args.push_back(std::string(argv[i]));
    }
    return run(args, std::cin, std::cout, std::cerr);
  } else if (command == "version") {
    printVersion(std::cerr);
    return 0;
  } else {
    printHelp(std::cerr);
    return 0;
  }
}

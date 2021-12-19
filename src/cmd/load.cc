//
//  load.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "load.h"

#include "src/cmd/util.h"
#include "src/common/graph.h"
#include "src/lang/representation/ast/ast_util.h"
#include "src/lang/representation/types/info_util.h"

namespace {

constexpr std::string_view kStdLibPath = "/Users/arne/Documents/Xcode/Katara/stdlib";

}

namespace cmd {

std::variant<LoadResult, ErrorCode> Load(const std::vector<std::string> args, std::ostream& err) {
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
        return ErrorCode::kLoadErrorMixedSourceFileArgsWithPackagePathArgs;
      }
      args_kind = ArgsKind::kMainPackageFiles;
      arg = path;
      args_without_flags.push_back(arg);
      continue;
    }

    if (std::filesystem::is_directory(path)) {
      if (args_kind != ArgsKind::kNone) {
        err << "can only handle one main package path argument\n";
        return ErrorCode::kLoadErrorMultiplePackagePathArgs;
      }
      args_kind = ArgsKind::kMainPackageDirectory;
      arg = path;
    } else {
      if (args_kind != ArgsKind::kNone && args_kind != ArgsKind::kPackagePaths) {
        err << "source file arguments can not be mixed with package path arguments\n";
        return ErrorCode::kLoadErrorMixedSourceFileArgsWithPackagePathArgs;
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

        WriteToFile(ast_graph.ToDotFormat(), debug_dir / (name + ".ast.dot"));
      }

      std::string type_info =
          lang::types::InfoToText(pkg_manager->file_set(), pkg_manager->type_info());
      WriteToFile(type_info, debug_dir / (main_pkg->name() + ".types.txt"));
    }
  }
  if (contains_issues) {
    return ErrorCode::kLoadErrorForPackage;
  }

  return LoadResult{
      .pkg_manager = std::move(pkg_manager),
      .arg_pkgs = arg_pkgs,
      .generate_debug_info = generate_debug_info,
  };
}

}  // namespace cmd

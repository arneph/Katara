//
//  load.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "load.h"

#include "src/common/graph.h"
#include "src/lang/representation/ast/ast_util.h"
#include "src/lang/representation/types/info_util.h"

namespace {

constexpr std::string_view kStdLibPath = "/Users/arne/Documents/Xcode/Katara/stdlib";

}

namespace cmd {

std::variant<LoadResult, ErrorCode> Load(Context* ctx) {
  enum class ArgsKind {
    kNone,
    kMainPackageDirectory,
    kMainPackageFiles,
    kPackagePaths,
  };
  ArgsKind args_kind = ArgsKind::kNone;
  for (std::string arg : ctx->args()) {
    std::filesystem::path path(arg);
    path = std::filesystem::absolute(path);
    if (path.extension() == ".kat") {
      if (args_kind != ArgsKind::kNone && args_kind != ArgsKind::kMainPackageFiles) {
        ctx->stderr() << "source file arguments can not be mixed with package path arguments\n";
        return ErrorCode::kLoadErrorMixedSourceFileArgsWithPackagePathArgs;
      }
      args_kind = ArgsKind::kMainPackageFiles;
      continue;
    }

    if (std::filesystem::is_directory(path)) {
      if (args_kind != ArgsKind::kNone) {
        ctx->stderr() << "can only handle one main package path argument\n";
        return ErrorCode::kLoadErrorMultiplePackagePathArgs;
      }
      args_kind = ArgsKind::kMainPackageDirectory;
    } else {
      if (args_kind != ArgsKind::kNone && args_kind != ArgsKind::kPackagePaths) {
        ctx->stderr() << "source file arguments can not be mixed with package path arguments\n";
        return ErrorCode::kLoadErrorMixedSourceFileArgsWithPackagePathArgs;
      }
      args_kind = ArgsKind::kPackagePaths;
    }
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
      main_pkg = pkg_manager->LoadMainPackage(ctx->args().front());
      break;
    case ArgsKind::kMainPackageFiles:
      main_pkg = pkg_manager->LoadMainPackage(ctx->args());
      break;
    case ArgsKind::kPackagePaths:
      for (std::string& pkg_path : ctx->args()) {
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
                                            ctx->stderr());
  for (auto pkg : pkg_manager->Packages()) {
    pkg->issue_tracker().PrintIssues(lang::issues::IssueTracker::PrintFormat::kTerminal,
                                     ctx->stderr());
    if (!pkg->issue_tracker().issues().empty()) {
      contains_issues = true;
    }
  }
  if (ctx->generate_debug_info()) {
    for (lang::packages::Package* pkg : arg_pkgs) {
      for (auto [name, ast_file] : pkg->ast_package()->files()) {
        common::Graph ast_graph = lang::ast::NodeToTree(pkg_manager->file_set(), ast_file);

        ctx->WriteToDebugFile(ast_graph.ToDotFormat(), name + ".ast.dot");
      }

      std::string type_info =
          lang::types::InfoToText(pkg_manager->file_set(), pkg_manager->type_info());
      ctx->WriteToDebugFile(type_info, main_pkg->name() + ".types.txt");
    }
  }
  if (contains_issues) {
    return ErrorCode::kLoadErrorForPackage;
  }

  return LoadResult{
      .pkg_manager = std::move(pkg_manager),
      .arg_pkgs = arg_pkgs,
  };
}

}  // namespace cmd

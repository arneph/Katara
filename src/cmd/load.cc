//
//  load.cc
//  Katara
//
//  Created by Arne Philipeit on 12/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "load.h"

#include "src/common/graph/graph.h"
#include "src/lang/representation/ast/ast_util.h"
#include "src/lang/representation/types/info_util.h"

namespace cmd {
namespace {

constexpr std::string_view kStdLibPath = "/Users/arne/Documents/Xcode/Katara/stdlib";

enum class ArgsKind {
  kNone,
  kMainPackageDirectory,
  kMainPackageFiles,
  kPackagePaths,
};

std::variant<ArgsKind, ErrorCode> FindArgsKind(Context* ctx) {
  ArgsKind args_kind = ArgsKind::kNone;
  for (std::string arg : ctx->args()) {
    std::filesystem::path path = ctx->filesystem()->Absolute(arg);
    if (path.extension() == ".kat") {
      if (args_kind != ArgsKind::kNone && args_kind != ArgsKind::kMainPackageFiles) {
        *ctx->stderr() << "source file arguments can not be mixed with package path arguments\n";
        return ErrorCode::kLoadErrorMixedSourceFileArgsWithPackagePathArgs;
      }
      args_kind = ArgsKind::kMainPackageFiles;
      continue;
    }

    if (ctx->filesystem()->IsDirectory(path)) {
      if (args_kind != ArgsKind::kNone) {
        *ctx->stderr() << "can only handle one main package path argument\n";
        return ErrorCode::kLoadErrorMultiplePackagePathArgs;
      }
      args_kind = ArgsKind::kMainPackageDirectory;
    } else {
      if (args_kind != ArgsKind::kNone && args_kind != ArgsKind::kPackagePaths) {
        *ctx->stderr() << "source file arguments can not be mixed with package path arguments\n";
        return ErrorCode::kLoadErrorMixedSourceFileArgsWithPackagePathArgs;
      }
      args_kind = ArgsKind::kPackagePaths;
    }
  }
  return args_kind;
}

std::vector<std::filesystem::path> ArgsToMainPackageFiles(std::vector<std::string>& args) {
  std::vector<std::filesystem::path> main_file_paths;
  main_file_paths.reserve(args.size());
  for (std::string& arg : args) {
    main_file_paths.push_back(arg);
  }
  return main_file_paths;
}

ErrorCode FindAndPrintIssues(std::unique_ptr<lang::packages::PackageManager>& pkg_manager,
                             Context* ctx) {
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
  if (contains_issues) {
    return ErrorCode::kLoadErrorForPackage;
  }
  return ErrorCode::kNoError;
}

void GenerateDebugInfo(std::unique_ptr<lang::packages::PackageManager>& pkg_manager,
                       lang::packages::Package* main_pkg,
                       std::vector<lang::packages::Package*>& arg_pkgs, Context* ctx) {
  if (!ctx->generate_debug_info()) {
    return;
  }

  for (lang::packages::Package* pkg : arg_pkgs) {
    for (auto [name, ast_file] : pkg->ast_package()->files()) {
      common::Graph ast_graph = lang::ast::NodeToTree(pkg_manager->file_set(), ast_file);

      ctx->WriteToDebugFile(ast_graph.ToDotFormat(), /* subdir_name= */ "", name + ".ast.dot");
    }

    std::string type_info =
        lang::types::InfoToText(pkg_manager->file_set(), pkg_manager->type_info());
    ctx->WriteToDebugFile(type_info, /* subdir_name= */ "", main_pkg->name() + ".types.txt");
  }
}

}  // namespace

std::variant<LoadResult, ErrorCode> Load(Context* ctx) {
  std::variant<ArgsKind, ErrorCode> args_kind_or_error = FindArgsKind(ctx);
  if (std::holds_alternative<ErrorCode>(args_kind_or_error)) {
    return std::get<ErrorCode>(args_kind_or_error);
  }
  ArgsKind args_kind = std::get<ArgsKind>(args_kind_or_error);
  auto pkg_manager = std::make_unique<lang::packages::PackageManager>(
      ctx->filesystem(), kStdLibPath, ctx->filesystem()->CurrentPath());
  lang::packages::Package* main_pkg = nullptr;
  std::vector<lang::packages::Package*> arg_pkgs;
  switch (args_kind) {
    case ArgsKind::kNone:
      main_pkg = pkg_manager->LoadMainPackage(ctx->filesystem()->CurrentPath());
      break;
    case ArgsKind::kMainPackageDirectory:
      main_pkg = pkg_manager->LoadMainPackage(ctx->args().front());
      break;
    case ArgsKind::kMainPackageFiles:
      main_pkg = pkg_manager->LoadMainPackage(ArgsToMainPackageFiles(ctx->args()));
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
  GenerateDebugInfo(pkg_manager, main_pkg, arg_pkgs, ctx);
  ErrorCode error_from_issues = FindAndPrintIssues(pkg_manager, ctx);
  if (error_from_issues != kNoError) {
    return error_from_issues;
  }
  return LoadResult{
      .pkg_manager = std::move(pkg_manager),
      .arg_pkgs = arg_pkgs,
  };
}

}  // namespace cmd

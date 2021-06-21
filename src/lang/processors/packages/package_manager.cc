//
//  package_manager.cc
//  Katara
//
//  Created by Arne Philipeit on 9/6/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "package_manager.h"

#include "src/lang/processors/parser/parser.h"
#include "src/lang/processors/type_checker/type_checker.h"
#include "src/lang/representation/ast/ast_util.h"

namespace lang {
namespace packages {

std::vector<Package*> PackageManager::Packages() const {
  std::vector<Package*> packages;
  for (auto& [_, package] : packages_) {
    packages.push_back(package.get());
  }
  return packages;
}

Package* PackageManager::GetPackage(std::string pkg_path) const {
  auto it = packages_.find(pkg_path);
  if (it != packages_.end()) {
    return it->second.get();
  }
  return nullptr;
}

Package* PackageManager::LoadPackage(std::string pkg_path) {
  if (Package* pkg = GetPackage(pkg_path); pkg != nullptr) {
    return pkg;
  }
  if (src_loader_->CanReadRelativeDir(pkg_path)) {
    std::vector<std::string> file_paths = src_loader_->SourceFilesInRelativeDir(pkg_path);
    return LoadPackage(pkg_path, src_loader_.get(), file_paths);
  } else if (stdlib_loader_->CanReadRelativeDir(pkg_path)) {
    std::vector<std::string> file_paths = stdlib_loader_->SourceFilesInRelativeDir(pkg_path);
    return LoadPackage(pkg_path, stdlib_loader_.get(), file_paths);
  } else {
    // TODO: handle error better
    return nullptr;
  }
}

Package* PackageManager::LoadMainPackage(std::string main_dir) {
  if (GetMainPackage() != nullptr) {
    throw "internal error: tried to load main package twice";
  }
  if (src_loader_->CanReadAbsoluteDir(main_dir)) {
    std::vector<std::string> file_paths = src_loader_->SourceFilesInAbsoluteDir(main_dir);
    return LoadPackage("main", src_loader_.get(), file_paths);
  } else {
    // TODO: handle error better
    return nullptr;
  }
}

Package* PackageManager::LoadMainPackage(std::vector<std::string> main_file_paths) {
  if (GetMainPackage() != nullptr) {
    throw "internal error: tried to load main package twice";
  }
  for (std::string file_path : main_file_paths) {
    if (!src_loader_->CanReadSourceFile(file_path)) {
      // TODO: handle error better
      return nullptr;
    }
  }
  return LoadPackage("main", src_loader_.get(), main_file_paths);
}

namespace {

std::string NameFromPath(std::string pkg_path) {
  size_t last_slash_index = pkg_path.find_last_of('/');
  if (last_slash_index == std::string::npos) {
    return pkg_path;
  } else {
    return pkg_path.substr(last_slash_index + 1);
  }
}

}  // namespace

Package* PackageManager::LoadPackage(std::string pkg_path, Loader* loader,
                                     std::vector<std::string> file_paths) {
  Package* pkg;
  if (auto [it, insert_ok] = packages_.insert({pkg_path, std::unique_ptr<Package>(new Package())});
      insert_ok) {
    pkg = it->second.get();
  } else {
    throw "internal error: tried to load package twice";
  }
  pkg->name_ = NameFromPath(pkg_path);
  pkg->path_ = pkg_path;
  if (file_paths.empty()) {
    pkg->issue_tracker_.Add(issues::kPackageDirectoryWithoutSourceFiles, std::vector<pos::pos_t>{},
                            "package directory does not contain source files");
    return pkg;
  }
  for (std::string file_path : file_paths) {
    std::string file_name = NameFromPath(file_path);
    std::string file_contents = loader->ReadSourceFile(file_path);
    pkg->pos_files_.push_back(file_set_.AddFile(file_name, file_contents));
  }

  ast::ASTBuilder ast_builder = ast_.builder();
  std::map<std::string, ast::File*> ast_files;
  for (pos::File* pos_file : pkg->pos_files_) {
    ast::File* ast_file = parser::Parser::ParseFile(pos_file, ast_builder, pkg->issue_tracker_);
    ast_files.insert({pos_file->name(), ast_file});
  }
  pkg->ast_package_ = ast_builder.CreatePackage(pkg->name_, ast_files);
  if (pkg->issue_tracker().has_fatal_errors()) {
    return pkg;
  }

  auto importer = [&](std::string import_path) -> types::Package* {
    Package* package = LoadPackage(import_path);
    if (package->issue_tracker().has_errors()) {
      return nullptr;
    }
    return package->types_package_;
  };
  types::Package* types_package =
      type_checker::Check(pkg_path, pkg->ast_package_, importer, type_info(), pkg->issue_tracker_);
  pkg->types_package_ = types_package;
  if (pkg->issue_tracker().has_fatal_errors()) {
    return pkg;
  }

  return pkg;
}

}  // namespace packages
}  // namespace lang

//
//  package_manager.cc
//  Katara
//
//  Created by Arne Philipeit on 9/6/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "package_manager.h"

#include <algorithm>

#include "src/common/logging/logging.h"
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
  if (filesystem_->Exists(src_path_ / pkg_path) && filesystem_->IsDirectory(src_path_ / pkg_path)) {
    return LoadPackage(pkg_path, src_path_ / pkg_path);
  } else if (filesystem_->Exists(stdlib_path_ / pkg_path) &&
             filesystem_->IsDirectory(stdlib_path_ / pkg_path)) {
    return LoadPackage(pkg_path, stdlib_path_ / pkg_path);
  }
  issue_tracker_.Add(issues::kPackageDirectoryNotFound, std::vector<pos::pos_t>{},
                     "package directory not found for: " + pkg_path);
  return nullptr;
}

Package* PackageManager::LoadMainPackage(std::filesystem::path main_directory) {
  if (GetMainPackage() != nullptr) {
    common::fail("tried to load main package twice");
  }
  if (!filesystem_->IsDirectory(main_directory)) {
    issue_tracker_.Add(issues::kMainPackageDirectoryUnreadable, std::vector<pos::pos_t>{},
                       "main package directory not readable: " + main_directory.string());
    return nullptr;
  }
  return LoadPackage("main", main_directory);
}

Package* PackageManager::LoadMainPackage(std::vector<std::filesystem::path> main_file_paths) {
  if (GetMainPackage() != nullptr) {
    common::fail("tried to load main package twice");
  }
  if (!CheckAllFilesAreInMainDirectory(main_file_paths) ||
      !CheckAllFilesInMainPackageExist(main_file_paths)) {
    return nullptr;
  }
  return LoadPackage("main", filesystem_->Absolute(main_file_paths.front().parent_path()),
                     main_file_paths);
}

bool PackageManager::CheckAllFilesAreInMainDirectory(
    std::vector<std::filesystem::path>& file_paths) {
  std::filesystem::path main_directory = filesystem_->Absolute(file_paths.front().parent_path());
  bool all_in_main_directory =
      std::all_of(file_paths.begin(), file_paths.end(), [&](std::filesystem::path file_path) {
        return filesystem_->Equivalent(main_directory, file_path.parent_path());
      });
  if (!all_in_main_directory) {
    issue_tracker_.Add(issues::kMainPackageFilesInMultipleDirectories, std::vector<pos::pos_t>{},
                       "main package files are not all in the same directory");
  }
  return all_in_main_directory;
}

bool PackageManager::CheckAllFilesInMainPackageExist(
    std::vector<std::filesystem::path>& file_paths) {
  bool all_readable = true;
  std::for_each(file_paths.begin(), file_paths.end(), [&](std::filesystem::path file_path) {
    if (!filesystem_->Exists(file_path) || filesystem_->IsDirectory(file_path)) {
      issue_tracker_.Add(issues::kMainPackageFileUnreadable, std::vector<pos::pos_t>{},
                         "main package file not readable: " + file_path.string());
      all_readable = false;
    }
  });
  return all_readable;
}

namespace {

std::string NameFromPackagePath(std::string pkg_path) {
  size_t last_slash_index = pkg_path.find_last_of('/');
  if (last_slash_index == std::string::npos) {
    return pkg_path;
  } else {
    return pkg_path.substr(last_slash_index + 1);
  }
}

}  // namespace

Package* PackageManager::LoadPackage(std::string pkg_path, std::filesystem::path pkg_directory) {
  std::vector<std::filesystem::path> file_paths;
  filesystem_->ForEntriesInDirectory(pkg_directory, [&](std::filesystem::path entry) {
    if (entry.extension() == ".kat" && !filesystem_->IsDirectory(entry)) {
      file_paths.push_back(entry);
    }
  });
  return LoadPackage(pkg_path, pkg_directory, file_paths);
}

Package* PackageManager::LoadPackage(std::string pkg_path, std::filesystem::path pkg_directory,
                                     std::vector<std::filesystem::path> file_paths) {
  Package* pkg;
  if (auto [it, insert_ok] =
          packages_.insert({pkg_path, std::unique_ptr<Package>(new Package(&file_set_))});
      insert_ok) {
    pkg = it->second.get();
  } else {
    common::fail("tried to load package twice");
  }
  pkg->name_ = NameFromPackagePath(pkg_path);
  pkg->path_ = pkg_path;
  pkg->directory_ = pkg_directory;
  if (file_paths.empty()) {
    pkg->issue_tracker_.Add(
        issues::kPackageDirectoryWithoutSourceFiles, std::vector<pos::pos_t>{},
        "package directory does not contain source files: " + pkg_directory.string());
    return pkg;
  }
  for (std::filesystem::path file_path : file_paths) {
    std::string file_name = file_path.filename();
    std::string file_contents = filesystem_->ReadContentsOfFile(file_path);
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

//
//  package_manager.h
//  Katara
//
//  Created by Arne Philipeit on 9/6/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_packages_package_manager_h
#define lang_packages_package_manager_h

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/common/filesystem/filesystem.h"
#include "src/common/positions/positions.h"
#include "src/lang/processors/issues/issues.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/types/info.h"

namespace lang {
namespace packages {

class PackageManager {
 public:
  PackageManager(common::Filesystem* filesystem, std::filesystem::path stdlib_path,
                 std::filesystem::path src_path)
      : filesystem_(filesystem),
        stdlib_path_(stdlib_path),
        src_path_(src_path),
        issue_tracker_(&file_set_) {}

  std::filesystem::path stdlib_path() const { return stdlib_path_; }
  std::filesystem::path src_path() const { return src_path_; }

  const common::PosFileSet* file_set() const { return &file_set_; }
  const issues::IssueTracker* issue_tracker() const { return &issue_tracker_; }
  const ast::AST* ast() const { return &ast_; }
  const types::Info* type_info() const { return &type_info_; }
  types::Info* type_info() { return &type_info_; }

  std::vector<Package*> Packages() const;
  // Returns the package with the given package path if it is already loaded, otherwise nullptr is
  // returned.
  Package* GetPackage(std::string pkg_path) const;
  // Returns the main package if it is already loaded, otherwise nullptr is returned.
  Package* GetMainPackage() const { return GetPackage("main"); }

  // Loads (if neccesary) and returns the package with the given package path, if successful. If
  // unsuccessful, nullptr gets returned and the encourted issue gets added to the package manager's
  // issue tracker.
  Package* LoadPackage(std::string pkg_path);
  // Loads and returns the main package in the given absolute package directory, if successful. If
  // unsuccessful, nullptr gets returned and the encourted issue gets added to the package manager's
  // issue tracker.
  Package* LoadMainPackage(std::filesystem::path main_directory);
  // Loads and returns the main package in the given absolute file paths, if successful. If
  // unsuccessful, nullptr gets returned and the encourted issue gets added to the package manager's
  // issue tracker.
  Package* LoadMainPackage(std::vector<std::filesystem::path> main_file_paths);

 private:
  bool CheckAllFilesAreInMainDirectory(std::vector<std::filesystem::path>& file_paths);
  bool CheckAllFilesInMainPackageExist(std::vector<std::filesystem::path>& file_paths);

  Package* LoadPackage(std::string pkg_path, std::filesystem::path pkg_directory);
  Package* LoadPackage(std::string pkg_path, std::filesystem::path pkg_directory,
                       std::vector<std::filesystem::path> file_paths);

  common::Filesystem* filesystem_;
  std::filesystem::path stdlib_path_;
  std::filesystem::path src_path_;

  common::PosFileSet file_set_;
  issues::IssueTracker issue_tracker_;
  ast::AST ast_;
  types::Info type_info_;
  std::unordered_map<std::string, std::unique_ptr<Package>> packages_;
};

}  // namespace packages
}  // namespace lang

#endif /* lang_packages_package_manager_h */

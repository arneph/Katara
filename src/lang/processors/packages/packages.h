//
//  packages.h
//  Katara
//
//  Created by Arne Philipeit on 9/6/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_packages_h
#define lang_packages_h

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

#include "src/lang/processors/issues/issues.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/positions/positions.h"
#include "src/lang/representation/types/info.h"
#include "src/lang/representation/types/package.h"

namespace lang {
namespace packages {

class Package {
 public:
  std::string name() const { return name_; }
  std::string path() const { return path_; }

  const std::vector<pos::File*>& pos_files() const { return pos_files_; }
  ast::Package* ast_package() const { return ast_package_; }
  types::Package* types_package() const { return types_package_; }

  const issues::IssueTracker& issue_tracker() const { return issue_tracker_; }

 private:
  Package() {}

  std::string name_;
  std::string path_;

  std::vector<pos::File*> pos_files_;
  ast::Package* ast_package_;
  types::Package* types_package_;

  issues::IssueTracker issue_tracker_;

  friend class PackageManager;
};

class PackageManager {
 public:
  PackageManager(std::string stdlib_path);

  pos::FileSet* file_set() const { return file_set_.get(); }
  ast::AST* ast() const { return ast_.get(); }
  types::Info* type_info() const { return type_info_.get(); }

  // GetPackage returns the package in the given package directory if is already loaded,
  // otherwise nullptr is returned.
  Package* GetPackage(std::string pkg_dir);
  // LoadPackage loads (if neccesary) and returns the package in the given package
  // directory.
  Package* LoadPackage(std::string pkg_dir);
  std::vector<Package*> Packages() const;

 private:
  std::filesystem::path FindPackagePath(std::string import, std::filesystem::path import_path);
  std::vector<std::filesystem::path> FindSourceFiles(std::filesystem::path package_path);

  std::filesystem::path stdlib_path_;
  std::unique_ptr<pos::FileSet> file_set_;
  std::unique_ptr<ast::AST> ast_;
  std::unique_ptr<types::Info> type_info_;
  std::unordered_map<std::string, std::unique_ptr<Package>> packages_;
};

}  // namespace packages
}  // namespace lang

#endif /* lang_packages_h */

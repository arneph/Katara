//
//  package.h
//  Katara
//
//  Created by Arne Philipeit on 6/19/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_packages_package_h
#define lang_packages_package_h

#include <filesystem>
#include <string>
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
  // Returns the name of the package, e.g. for images/png, the package name is "png".
  std::string name() const { return name_; }
  // Returns the path of the package as used to identify the package in import statements.
  std::string path() const { return path_; }
  // Returns the absolute directory containing the package, e.g. to write debug information to.
  std::filesystem::path directory() const { return directory_; }

  const std::vector<pos::File*>& pos_files() const { return pos_files_; }
  ast::Package* ast_package() const { return ast_package_; }
  types::Package* types_package() const { return types_package_; }

  const issues::IssueTracker& issue_tracker() const { return issue_tracker_; }

 private:
  Package(const pos::FileSet* file_set)
      : ast_package_(nullptr), types_package_(nullptr), issue_tracker_(file_set) {}

  std::string name_;
  std::string path_;
  std::filesystem::path directory_;

  std::vector<pos::File*> pos_files_;
  ast::Package* ast_package_;
  types::Package* types_package_;

  issues::IssueTracker issue_tracker_;

  friend class PackageManager;
};

}  // namespace packages
}  // namespace lang

#endif /* lang_packages_package_h */

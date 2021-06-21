//
//  package_manager.h
//  Katara
//
//  Created by Arne Philipeit on 9/6/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_packages_package_manager_h
#define lang_packages_package_manager_h

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/lang/processors/packages/filesystem_loader.h"
#include "src/lang/processors/packages/loader.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/positions/positions.h"
#include "src/lang/representation/types/info.h"

namespace lang {
namespace packages {

class PackageManager {
 public:
  PackageManager(std::string stdlib_dir, std::string src_dir)
      : PackageManager(std::make_unique<FilesystemLoader>(stdlib_dir),
                       std::make_unique<FilesystemLoader>(src_dir)) {}
  PackageManager(std::unique_ptr<Loader> stdlib_loader, std::unique_ptr<Loader> src_loader)
      : stdlib_loader_(std::move(stdlib_loader)), src_loader_(std::move(src_loader)) {}

  const pos::FileSet* file_set() const { return &file_set_; }
  const ast::AST* ast() const { return &ast_; }
  const types::Info* type_info() const { return &type_info_; }
  types::Info* type_info() { return &type_info_; }

  std::vector<Package*> Packages() const;
  // Returns the package with the given package path if it is already loaded, otherwise nullptr is
  // returned.
  Package* GetPackage(std::string pkg_path) const;
  // Returns the main package if it is already loaded, otherwise nullptr is returned.
  Package* GetMainPackage() const { return GetPackage("main"); }

  // Loads (if neccesary) and returns the package with the given package path.
  Package* LoadPackage(std::string pkg_path);
  // Loads and returns the main package in the given absolute package directory.
  Package* LoadMainPackage(std::string main_dir);
  // Loads and returns the main package in the given absolute file paths.
  Package* LoadMainPackage(std::vector<std::string> main_file_paths);

 private:
  Package* LoadPackage(std::string pkg_path, Loader* loader, std::vector<std::string> file_paths);

  std::unique_ptr<Loader> stdlib_loader_;
  std::unique_ptr<Loader> src_loader_;

  pos::FileSet file_set_;
  ast::AST ast_;
  types::Info type_info_;
  std::unordered_map<std::string, std::unique_ptr<Package>> packages_;
};

}  // namespace packages
}  // namespace lang

#endif /* lang_packages_package_manager_h */

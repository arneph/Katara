//
//  type_checker.cc
//  Katara
//
//  Created by Arne Philipeit on 7/26/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "type_checker.h"

#include "lang/processors/type_checker/coordinator.h"
#include "lang/processors/type_checker/identifier_resolver.h"
#include "lang/representation/types/info_builder.h"

namespace lang {
namespace type_checker {

types::Package* Check(std::string package_path, ast::Package* ast_package,
                      std::function<types::Package*(std::string)> importer, types::Info* info,
                      std::vector<issues::Issue>& issues) {
  std::vector<ast::File*> ast_files;
  for (auto [name, ast_file] : ast_package->files()) {
    ast_files.push_back(ast_file);
  }
  types::InfoBuilder info_builder = info->builder();
  info_builder.CreateUniverse();

  types::Package* types_package = IdentifierResolver::CreatePackageAndResolveIdentifiers(
      package_path, ast_files, importer, info_builder, issues);
  for (issues::Issue& issue : issues) {
    if (issue.origin() == issues::Origin::TypeChecker &&
        issue.severity() == issues::Severity::Fatal) {
      return nullptr;
    }
  }

  bool ok = Coordinator::ProcessPackage(ast_files, types_package, info_builder, issues);
  if (!ok) {
    return nullptr;
  }

  return types_package;
}

}  // namespace type_checker
}  // namespace lang

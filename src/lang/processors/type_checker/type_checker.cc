//
//  type_checker.cc
//  Katara
//
//  Created by Arne Philipeit on 7/26/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "type_checker.h"

#include "src/lang/processors/issues/issues.h"
#include "src/lang/processors/type_checker/coordinator.h"
#include "src/lang/processors/type_checker/identifier_resolver.h"
#include "src/lang/representation/types/info_builder.h"

namespace lang {
namespace type_checker {

using ::common::issues::Severity;

types::Package* Check(std::string package_path, ast::Package* ast_package,
                      std::function<types::Package*(std::string)> importer, types::Info* info,
                      issues::IssueTracker& issues) {
  std::vector<ast::File*> ast_files;
  for (auto [name, ast_file] : ast_package->files()) {
    ast_files.push_back(ast_file);
  }
  types::InfoBuilder info_builder = info->builder();
  info_builder.CreateUniverse();

  types::Package* types_package = IdentifierResolver::CreatePackageAndResolveIdentifiers(
      package_path, ast_files, importer, info_builder, issues);
  for (const issues::Issue& issue : issues.issues()) {
    if (issue.origin() == issues::Origin::kIdentifierResolver &&
        issue.severity() == Severity::kFatal) {
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

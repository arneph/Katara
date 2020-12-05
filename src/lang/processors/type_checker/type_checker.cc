//
//  type_checker.cc
//  Katara
//
//  Created by Arne Philipeit on 7/26/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "type_checker.h"

#include "lang/representation/types/info_builder.h"
#include "lang/processors/type_checker/identifier_resolver.h"
#include "lang/processors/type_checker/package_handler.h"

namespace lang {
namespace type_checker {

types::Package * Check(std::string package_path,
                       std::vector<ast::File *> package_files,
                       std::function<types::Package *(std::string)> importer,
                       types::Info *info,
                       std::vector<issues::Issue>& issues) {
    types::InfoBuilder info_builder = info->builder();
    info_builder.CreateUniverse();
    
    types::Package *package =
        IdentifierResolver::CreatePackageAndResolveIdentifiers(package_path,
                                                               package_files,
                                                               importer,
                                                               info_builder,
                                                               issues);
    for (issues::Issue& issue : issues) {
        if (issue.origin() == issues::Origin::TypeChecker &&
            issue.severity() == issues::Severity::Fatal) {
            return nullptr;
        }
    }
    
    bool ok = PackageHandler::ProcessPackage(package_files, package, info_builder, issues);
    if (!ok) {
        return nullptr;
    }
    
    return package;
}

}
}

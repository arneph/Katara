//
//  type_checker.cc
//  Katara
//
//  Created by Arne Philipeit on 7/26/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "type_checker.h"

#include "lang/processors/type_checker/universe_builder.h"
#include "lang/processors/type_checker/identifier_resolver.h"
#include "lang/processors/type_checker/init_handler.h"
#include "lang/processors/type_checker/constant_handler.h"

namespace lang {
namespace type_checker {

types::Package * Check(std::string package_path,
                       std::vector<ast::File *> package_files,
                       std::function<types::Package *(std::string)> importer,
                       types::TypeInfo *type_info,
                       std::vector<issues::Issue>& issues) {
    UniverseBuilder::SetupUniverse(type_info);
    
    types::Package *package =
        IdentifierResolver::CreatePackageAndResolveIdentifiers(package_path,
                                                               package_files,
                                                               importer,
                                                               type_info,
                                                               issues);
    for (issues::Issue& issue : issues) {
        if (issue.origin() == issues::Origin::TypeChecker &&
            issue.severity() == issues::Severity::Fatal) {
            return nullptr;
        }
    }
    
    InitHandler::HandleInits(package_files,
                             package,
                             type_info,
                             issues);
    
    ConstantHandler::HandleConstants(package_files,
                                     package,
                                     type_info,
                                     issues);
    
    return package;
}

}
}

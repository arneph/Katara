//
//  init_handler.h
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_init_handler_h
#define lang_type_checker_init_handler_h

#include <map>
#include <unordered_set>

#include "lang/representation/ast/ast.h"
#include "lang/representation/types/types.h"
#include "lang/processors/issues/issues.h"

namespace lang {
namespace type_checker {

class InitHandler {
public:
    static void HandleInits(std::vector<ast::File *> package_files,
                            types::Package *package,
                            types::TypeInfo *info,
                            std::vector<issues::Issue>& issues);
    
private:
    InitHandler(std::vector<ast::File *> package_files,
                types::Package *package,
                types::TypeInfo *info,
                std::vector<issues::Issue>& issues)
        : package_files_(package_files), package_(package), info_(info), issues_(issues) {}
    
    void FindInitOrder();
    void FindInitializersAndDependencies(std::map<types::Variable *, types::Initializer *>& inits,
                                         std::map<types::Object *,
                                         std::unordered_set<types::Object *>>& deps);
    std::unordered_set<types::Object *> FindInitDependenciesOfNode(ast::Node *node);

    std::vector<ast::File *> package_files_;
    types::Package *package_;
    types::TypeInfo *info_;
    std::vector<issues::Issue>& issues_;
};

}
}

#endif /* lang_type_checker_init_handler_h */

//
//  variable_handler.h
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_variable_handler_h
#define lang_type_checker_variable_handler_h

#include <memory>

#include "lang/representation/ast/ast.h"
#include "lang/representation/types/types.h"
#include "lang/processors/issues/issues.h"

namespace lang {
namespace type_checker {

class VariableHandler {
public:
    static bool ProcessVariable(types::Variable * variable,
                                types::Type *type,
                                ast::Expr *value,
                                types::TypeInfo *info,
                                std::vector<issues::Issue>& issues);
    static bool ProcessVariables(std::vector<types::Variable *> variables,
                                 types::Type *type,
                                 ast::Expr *value,
                                 types::TypeInfo *info,
                                 std::vector<issues::Issue>& issues);
    
private:
    VariableHandler(types::TypeInfo *info,
                    std::vector<issues::Issue>& issues)
    : info_(info), issues_(issues) {}
    
    bool ProcessVariableDefinitions(std::vector<types::Variable *> variables,
                                    types::Type *type,
                                    ast::Expr *value);
    
    types::TypeInfo *info_;
    std::vector<issues::Issue>& issues_;
};

}
}

#endif /* lang_type_checker_variable_handler_h */

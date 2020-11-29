//
//  variable_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "variable_handler.h"

#include "lang/representation/types/types_util.h"
#include "lang/processors/type_checker/expr_handler.h"

namespace lang {
namespace type_checker {

bool VariableHandler::ProcessVariable(types::Variable *variable,
                                      types::Type *type,
                                      ast::Expr *value_expr,
                                      types::TypeInfo *info,
                                      std::vector<issues::Issue> &issues) {
    return ProcessVariables(std::vector<types::Variable *>{variable},
                            type,
                            value_expr,
                            info,
                            issues);
}

bool VariableHandler::ProcessVariables(std::vector<types::Variable *> variables,
                                       types::Type *type,
                                       ast::Expr *value_expr,
                                       types::TypeInfo *info,
                                       std::vector<issues::Issue> &issues) {
    VariableHandler handler(info, issues);
    
    return handler.ProcessVariableDefinitions(variables, type, value_expr);
}

bool VariableHandler::ProcessVariableDefinitions(std::vector<types::Variable *> variables,
                                                 types::Type *type,
                                                 ast::Expr *value) {
    if (type == nullptr && value == nullptr) {
        for (auto variable :variables) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            variable->position(),
                                            "variable needs a type or value: "
                                            + variable->name()));
        }
        return false;
    }
    
    if (type != nullptr) {
        for (auto variable : variables) {
            variable->type_ = type;
        }
    }
    if (value == nullptr) {
        return true;
    }
    
    if (!ExprHandler::ProcessExpr(value, info_, issues_)) {
        return false;
    }
    
    types::Type *value_type = info_->types_.at(value);
    if (variables.size() == 1) {
        types::Variable * variable = variables.at(0);
        if (type != nullptr &&
            !types::IsAssignableTo(value_type, type)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            variable->position(),
                                            "variable can not be assigned given value: "
                                            + variable->name()));
            return false;
        }
        if (type == nullptr) {
            variable->type_ = value_type;
        }
        if (variable->parent() == variable->package()->scope()) {
            auto initializer =
                std::unique_ptr<types::Initializer>(new types::Initializer());
            initializer->lhs_.push_back(variable);
            initializer->rhs_ = value;
            
            auto initializer_ptr = initializer.get();
            info_->initializer_unique_ptrs_.push_back(std::move(initializer));
            info_->init_order_.push_back(initializer_ptr);
        }
        return true;
    }
    
    types::Tuple *tuple = dynamic_cast<types::Tuple *>(value_type);
    if (tuple == nullptr ||
        tuple->variables().size() != variables.size()) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        variables.at(0)->position(),
                                        "variables can not be assigned given value"));
        return false;
    }
    
    for (size_t i = 0; i < variables.size(); i++) {
        types::Variable * variable = variables.at(i);
        types::Type * value_type = tuple->variables().at(i)->type();
        
        if (type != nullptr &&
            !types::IsAssignableTo(value_type, type)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            variable->position(),
                                            "variable can not be assigned given value: "
                                            + variable->name()));
            return false;
        }
        if (type == nullptr) {
            variable->type_ = value_type;
        }
    }
    if (variables.at(0)->parent() == variables.at(0)->package()->scope()) {
        auto initializer =
        std::unique_ptr<types::Initializer>(new types::Initializer());
        initializer->lhs_ = variables;
        initializer->rhs_ = value;
        
        auto initializer_ptr = initializer.get();
        info_->initializer_unique_ptrs_.push_back(std::move(initializer));
        info_->init_order_.push_back(initializer_ptr);
    }
    return true;
}

}
}

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
                                      types::InfoBuilder& info_builder,
                                      std::vector<issues::Issue> &issues) {
    return ProcessVariables(std::vector<types::Variable *>{variable},
                            type,
                            value_expr,
                            info_builder,
                            issues);
}

bool VariableHandler::ProcessVariables(std::vector<types::Variable *> variables,
                                       types::Type *type,
                                       ast::Expr *value_expr,
                                       types::InfoBuilder& info_builder,
                                       std::vector<issues::Issue> &issues) {
    VariableHandler handler(info_builder, issues);
    
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
            info_builder_.SetObjectType(variable, type);
        }
    }
    if (value == nullptr) {
        return true;
    }
    
    if (!ExprHandler::ProcessExpr(value, info_builder_, issues_)) {
        return false;
    }
    
    types::ExprInfo value_info = info_->ExprInfoOf(value).value();
    if (!value_info.is_value()) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        value->start(),
                                        "expression is not a value"));
        return false;
    }
    if (variables.size() == 1) {
        types::Variable * variable = variables.at(0);
        if (type != nullptr &&
            !types::IsAssignableTo(value_info.type(), type)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            variable->position(),
                                            "variable can not be assigned given value: "
                                            + variable->name()));
            return false;
        }
        if (type == nullptr) {
            info_builder_.SetObjectType(variable, value_info.type());
        }
        if (variable->parent() == variable->package()->scope()) {
            info_builder_.AddInitializer(types::Initializer({variable}, value));
        }
        return true;
    }
    
    if (value_info.type()->type_kind() != types::TypeKind::kTuple ||
        static_cast<types::Tuple *>(value_info.type())->variables().size() != variables.size()) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        variables.at(0)->position(),
                                        "variables can not be assigned given value"));
        return false;
    }
    
    for (size_t i = 0; i < variables.size(); i++) {
        types::Variable *var = variables.at(i);
        types::Type *var_type =
            static_cast<types::Tuple *>(value_info.type())->variables().at(i)->type();
        
        if (type != nullptr && !types::IsAssignableTo(var_type, type)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            var->position(),
                                            "variable can not be assigned given value: "
                                            + var->name()));
            return false;
        }
        if (type == nullptr) {
            info_builder_.SetObjectType(var, var_type);
        }
    }
    if (variables.at(0)->parent() == variables.at(0)->package()->scope()) {
        info_builder_.AddInitializer(types::Initializer(variables, value));
    }
    return true;
}

}
}

//
//  variable_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "variable_handler.h"

#include "lang/processors/type_checker/type_resolver.h"
#include "lang/representation/types/types_util.h"

namespace lang {
namespace type_checker {

bool VariableHandler::ProcessVariable(types::Variable* variable, types::Type* type,
                                      ast::Expr* value_expr) {
  return ProcessVariables(std::vector<types::Variable*>{variable}, type, value_expr);
}

bool VariableHandler::ProcessVariables(std::vector<types::Variable*> variables, types::Type* type,
                                       ast::Expr* value_expr) {
  return ProcessVariableDefinitions(variables, type, value_expr);
}

bool VariableHandler::ProcessVariableDefinitions(std::vector<types::Variable*> variables,
                                                 types::Type* variable_type, ast::Expr* value) {
  if (variable_type == nullptr && value == nullptr) {
    for (auto variable : variables) {
      issues().Add(issues::kMissingTypeOrValueForVariable, variable->position(),
                   "variable needs a type or value: " + variable->name());
    }
    return false;
  }

  if (variable_type != nullptr) {
    for (auto variable : variables) {
      info_builder().SetObjectType(variable, variable_type);
    }
  }
  if (value == nullptr) {
    return true;
  } else if (!type_resolver().expr_handler().CheckValueExpr(value)) {
    return false;
  }

  types::Type* value_type = info()->ExprInfoOf(value).value().type();
  if (variables.size() == 1) {
    types::Variable* variable = variables.at(0);
    if (variable_type != nullptr && !types::IsAssignableTo(value_type, variable_type)) {
      issues().Add(issues::kVariableValueOfWrongType, variable->position(),
                   "variable can not be assigned given value: " + variable->name());
      return false;
    }
    if (variable_type == nullptr) {
      info_builder().SetObjectType(variable, value_type);
    }
    if (variable->parent() == variable->package()->scope()) {
      info_builder().AddInitializer(types::Initializer({variable}, value));
    }
    return true;
  }

  // TODO: handle ValueOk
  if (value_type->type_kind() != types::TypeKind::kTuple ||
      static_cast<types::Tuple*>(value_type)->variables().size() != variables.size()) {
    issues().Add(issues::kVariableValueOfWrongType, variables.at(0)->position(),
                 "variables can not be assigned given value");
    return false;
  }

  for (size_t i = 0; i < variables.size(); i++) {
    types::Variable* var = variables.at(i);
    types::Type* val_type = static_cast<types::Tuple*>(value_type)->variables().at(i)->type();

    if (variable_type != nullptr && !types::IsAssignableTo(val_type, variable_type)) {
      issues().Add(issues::kVariableValueOfWrongType, var->position(),
                   "variable can not be assigned given value: " + var->name());
      return false;
    }
    if (variable_type == nullptr) {
      info_builder().SetObjectType(var, val_type);
    }
  }
  if (variables.at(0)->parent() == variables.at(0)->package()->scope()) {
    info_builder().AddInitializer(types::Initializer(variables, value));
  }
  return true;
}

}  // namespace type_checker
}  // namespace lang

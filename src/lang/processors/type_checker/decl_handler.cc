//
//  decl_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 3/28/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "decl_handler.h"

#include "lang/processors/type_checker/type_resolver.h"
#include "lang/representation/types/types_util.h"

namespace lang {
namespace type_checker {

bool DeclHandler::ProcessTypeName(types::TypeName* type_name, ast::TypeSpec* type_spec) {
  if (!ProcessTypeParametersOfTypeName(type_name, type_spec)) {
    return false;
  }
  return ProcessUnderlyingTypeOfTypeName(type_name, type_spec);
}

bool DeclHandler::ProcessTypeParametersOfTypeName(types::TypeName* type_name,
                                                  ast::TypeSpec* type_spec) {
  std::vector<types::TypeParameter*> type_parameters;
  if (type_spec->type_params() != nullptr) {
    type_parameters =
        type_resolver().type_handler().EvaluateTypeParameters(type_spec->type_params());
    if (type_parameters.empty()) {
      return false;
    }
  }
  types::NamedType* named_type = static_cast<types::NamedType*>(type_name->type());
  info_builder().SetTypeParametersOfNamedType(named_type, type_parameters);
  return true;
}

bool DeclHandler::ProcessUnderlyingTypeOfTypeName(types::TypeName* type_name,
                                                  ast::TypeSpec* type_spec) {
  types::Type* underlying_type = type_resolver().type_handler().EvaluateTypeExpr(type_spec->type());
  if (underlying_type == nullptr) {
    return false;
  }
  types::NamedType* named_type = static_cast<types::NamedType*>(type_name->type());
  info_builder().SetUnderlyingTypeOfNamedType(named_type, underlying_type);
  return true;
}

bool DeclHandler::ProcessConstant(types::Constant* constant, ast::Expr* type_expr,
                                  ast::Expr* value_expr, int64_t iota) {
  types::Type *type = nullptr;
  if (type_expr != nullptr) {
    type = type_resolver().type_handler().EvaluateTypeExpr(type_expr);
    if (type == nullptr) {
      return false;
    }
  }
  if (type == nullptr && value_expr == nullptr) {
    issues().Add(issues::kMissingTypeOrValueForConstant, constant->position(),
                 "constant needs a type or value: " + constant->name());
    return false;
  }

  types::Basic* basic_type = nullptr;
  constants::Value value(int64_t{0});

  if (type != nullptr) {
    types::Type* underlying = types::UnderlyingOf(type);
    if (underlying == nullptr || underlying->type_kind() != types::TypeKind::kBasic) {
      issues().Add(issues::kConstantWithNonBasicType, constant->position(),
                   "constant can not have non-basic type: " + constant->name());
      return false;
    }
    basic_type = static_cast<types::Basic*>(underlying);
  }

  if (value_expr == nullptr) {
    value = types::ConvertUntypedValue(value, basic_type->kind());

  } else {
    if (!type_resolver().expr_handler().CheckExpr(
            value_expr, ExprHandler::Context(/*expect_constant=*/true, iota))) {
      return false;
    }
    types::ExprInfo value_expr_info = info()->ExprInfoOf(value_expr).value();
    types::Basic* given_type = static_cast<types::Basic*>(value_expr_info.type());
    constants::Value given_value = value_expr_info.constant_value();

    if (basic_type == nullptr) {
      type = given_type;
      basic_type = given_type;
    }

    if (given_type == basic_type) {
      value = given_value;

    } else if (given_type->info() & types::Basic::kIsUntyped) {
      value = types::ConvertUntypedValue(given_value, basic_type->kind());

    } else {
      issues().Add(issues::kConstantValueOfWrongType, constant->position(),
                   "constant can not have a value of a different type: " + constant->name());
      return false;
    }
  }

  info_builder().SetObjectType(constant, type);
  info_builder().SetConstantValue(constant, value);
  return true;
}

bool DeclHandler::ProcessVariable(types::Variable* variable, ast::Expr* variable_type_expr,
                                  ast::Expr* value_expr) {
  return ProcessVariables(std::vector<types::Variable*>{variable}, variable_type_expr, value_expr);
}

bool DeclHandler::ProcessVariables(std::vector<types::Variable*> variables,
                                   ast::Expr* all_variables_type_expr, ast::Expr* value_expr) {
  types::Type *all_variables_type = nullptr;
  if (all_variables_type_expr != nullptr) {
    all_variables_type = type_resolver().type_handler().EvaluateTypeExpr(all_variables_type_expr);
  }
  if (all_variables_type == nullptr && value_expr == nullptr) {
    if (variables.size() == 1) {
      issues().Add(issues::kMissingTypeOrValueForVariable, variables.front()->position(),
                   "variable needs a type or value: " + variables.front()->name());
    } else {
      issues().Add(issues::kMissingTypeOrValueForVariable, variables.front()->position(),
                   "variables needs a type or value");
    }
    return false;
  }

  if (all_variables_type != nullptr) {
    for (auto variable : variables) {
      info_builder().SetObjectType(variable, all_variables_type);
    }
  }
  if (value_expr == nullptr) {
    return true;
  }
  
  types::Type* value_type = type_resolver().expr_handler().CheckValueExpr(value_expr);
  if (value_type == nullptr) {
    return false;
  }

  if (variables.size() == 1) {
    types::Variable* variable = variables.at(0);
    if (all_variables_type != nullptr && !types::IsAssignableTo(value_type, all_variables_type)) {
      issues().Add(issues::kVariableValueOfWrongType, variable->position(),
                   "variable can not be assigned given value: " + variable->name());
      return false;
    }
    if (all_variables_type == nullptr) {
      info_builder().SetObjectType(variable, value_type);
    }
    if (variable->parent() == variable->package()->scope()) {
      info_builder().AddInitializer(types::Initializer({variable}, value_expr));
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

    if (all_variables_type != nullptr && !types::IsAssignableTo(val_type, all_variables_type)) {
      issues().Add(issues::kVariableValueOfWrongType, var->position(),
                   "variable can not be assigned given value: " + var->name());
      return false;
    }
    if (all_variables_type == nullptr) {
      info_builder().SetObjectType(var, val_type);
    }
  }
  if (variables.at(0)->parent() == variables.at(0)->package()->scope()) {
    info_builder().AddInitializer(types::Initializer(variables, value_expr));
  }
  return true;
}

bool DeclHandler::ProcessFunction(types::Func* func, ast::FuncDecl* func_decl) {
  if (func_decl->kind() != ast::FuncDecl::Kind::kFunc && func_decl->type_params()) {
    issues().Add(issues::kForbiddenTypeParameterDeclarationForMethod, func_decl->start(),
                 "method can not declare type parameters");
  }

  types::Variable* expr_receiver = nullptr;
  types::Type* type_receiver = nullptr;
  if (func_decl->kind() == ast::FuncDecl::Kind::kInstanceMethod) {
    expr_receiver = EvaluateExprReceiver(func_decl->expr_receiver(), func);
    if (expr_receiver == nullptr) {
      return false;
    }
  } else if (func_decl->kind() == ast::FuncDecl::Kind::kTypeMethod) {
    type_receiver = EvaluateTypeReceiver(func_decl->type_receiver(), func);
    if (type_receiver == nullptr) {
      return false;
    }
  }

  std::vector<types::TypeParameter*> type_parameters;
  if (func_decl->type_params() != nullptr) {
    type_parameters =
        type_resolver().type_handler().EvaluateTypeParameters(func_decl->type_params());
    if (type_parameters.empty()) {
      return false;
    }
  }

  types::Tuple* parameters =
      type_resolver().type_handler().EvaluateTuple(func_decl->func_type()->params());
  types::Tuple* results = nullptr;
  if (func_decl->func_type()->results() != nullptr) {
    results = type_resolver().type_handler().EvaluateTuple(func_decl->func_type()->results());
    if (results == nullptr) {
      return false;
    }
  }

  types::Signature* signature;
  if (expr_receiver != nullptr) {
    signature = info_builder().CreateSignature(expr_receiver, parameters, results);
  } else if (type_receiver != nullptr) {
    signature = info_builder().CreateSignature(type_receiver, parameters, results);
  } else {
    signature = info_builder().CreateSignature(type_parameters, parameters, results);
  }
  info_builder().SetObjectType(func, signature);
  return true;
}

types::Variable* DeclHandler::EvaluateExprReceiver(ast::ExprReceiver* expr_receiver,
                                                   types::Func* method) {
  types::Type* type = EvalutateReceiverTypeInstance(expr_receiver->type_name(),
                                                    expr_receiver->type_parameter_names(), method);
  if (type == nullptr) {
    return nullptr;
  }

  if (expr_receiver->pointer() != tokens::kIllegal) {
    types::Pointer::Kind kind;
    switch (expr_receiver->pointer()) {
      case tokens::kMul:
        kind = types::Pointer::Kind::kStrong;
        break;
      case tokens::kRem:
        kind = types::Pointer::Kind::kWeak;
      default:
        throw "unexpected pointer type";
    }

    types::Pointer* pointer_type = info_builder().CreatePointer(kind, type);
    type = pointer_type;
  }

  types::Variable* receiver;
  if (expr_receiver->name() != nullptr) {
    receiver = static_cast<types::Variable*>(info()->DefinitionOf(expr_receiver->name()));
  } else {
    receiver = static_cast<types::Variable*>(info()->ImplicitOf(expr_receiver));
  }
  info_builder().SetObjectType(receiver, type);
  return receiver;
}

types::Type* DeclHandler::EvaluateTypeReceiver(ast::TypeReceiver* type_receiver,
                                               types::Func* method) {
  return EvalutateReceiverTypeInstance(type_receiver->type_name(),
                                       type_receiver->type_parameter_names(), method);
}

types::Type* DeclHandler::EvalutateReceiverTypeInstance(ast::Ident* type_name_ident,
                                                        std::vector<ast::Ident*> type_param_names,
                                                        types::Func* method) {
  types::TypeName* type_name = static_cast<types::TypeName*>(info()->UseOf(type_name_ident));
  if (type_name->type()->type_kind() != types::TypeKind::kNamedType) {
    issues().Add(issues::kReceiverOfNonNamedType, type_name_ident->start(),
                 "receiver does not have named type");
    return nullptr;
  }
  types::NamedType* named_type = static_cast<types::NamedType*>(type_name->type());
  if (named_type->underlying()->type_kind() == types::TypeKind::kInterface) {
    issues().Add(issues::kDefinitionOfInterfaceMethodOutsideInterface, type_name_ident->start(),
                 "can not define additional methods for interfaces");
    return nullptr;
  } else if (named_type->methods().contains(method->name())) {
    types::Func* other_method = named_type->methods().at(method->name());
    issues().Add(issues::kRedefinitionOfMethod, {other_method->position(), method->position()},
                 "can not define two methods with the same name");
    return nullptr;
  }
  info_builder().AddMethodToNamedType(named_type, method);

  if (type_param_names.size() != named_type->type_parameters().size()) {
    issues().Add(issues::kWrongNumberOfTypeArgumentsForReceiver, type_name_ident->start(),
                 "receiver has wrong number of type arguments");
    return nullptr;
  }
  if (!named_type->type_parameters().empty()) {
    std::vector<types::Type*> type_instance_args;
    type_instance_args.reserve(named_type->type_parameters().size());
    for (size_t i = 0; i < named_type->type_parameters().size(); i++) {
      types::TypeParameter* instantiated = named_type->type_parameters().at(i);
      ast::Ident* arg_name = type_param_names.at(i);
      types::TypeName* arg = static_cast<types::TypeName*>(info()->DefinitionOf(arg_name));
      types::TypeParameter* instance = static_cast<types::TypeParameter*>(arg->type());
      info_builder().SetTypeParameterInstance(instantiated, instance);
      type_instance_args.push_back(instance);
    }
    return info_builder().CreateTypeInstance(named_type, type_instance_args);
  }
  return named_type;
}

}  // namespace type_checker
}  // namespace lang

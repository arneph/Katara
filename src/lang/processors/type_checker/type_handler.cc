//
//  type_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "type_handler.h"

#include <algorithm>

#include "src/lang/processors/type_checker/type_resolver.h"
#include "src/lang/representation/types/types_util.h"

namespace lang {
namespace type_checker {

std::vector<types::Type*> TypeHandler::EvaluateTypeExprs(const std::vector<ast::Expr*>& exprs) {
  std::vector<types::Type*> types;
  types.reserve(exprs.size());
  bool ok = true;
  for (ast::Expr* expr : exprs) {
    types::Type* type = EvaluateTypeExpr(expr);
    if (type == nullptr) {
      ok = false;
      types.clear();
    } else {
      types.push_back(type);
    }
  }
  return types;
}

types::Type* TypeHandler::EvaluateTypeExpr(ast::Expr* expr) {
  switch (expr->node_kind()) {
    case ast::NodeKind::kIdent:
      return EvaluateTypeIdent(static_cast<ast::Ident*>(expr));
    case ast::NodeKind::kParenExpr: {
      ast::ParenExpr* paren_expr = static_cast<ast::ParenExpr*>(expr);
      types::Type* x_type = EvaluateTypeExpr(paren_expr->x());
      if (x_type == nullptr) {
        return nullptr;
      }
      info_builder().SetExprInfo(paren_expr, types::ExprInfo(types::ExprInfo::Kind::kType, x_type));
      return x_type;
    }
    case ast::NodeKind::kSelectionExpr: {
      ast::SelectionExpr* selector_expr = static_cast<ast::SelectionExpr*>(expr);
      if (selector_expr->accessed()->node_kind() != ast::NodeKind::kIdent) {
        issues().Add(issues::kForbiddenTypeExpression, expr->start(),
                     "type expression not allowed");
        return nullptr;
      }
      ast::Ident* ident = static_cast<ast::Ident*>(selector_expr->accessed());
      if (info()->uses().at(ident)->object_kind() != types::ObjectKind::kPackageName) {
        issues().Add(issues::kForbiddenTypeExpression, expr->start(),
                     "type expression not allowed");
        return nullptr;
      }
      return EvaluateTypeIdent(selector_expr->selection());
    }
    case ast::NodeKind::kUnaryExpr:
      return EvaluatePointerType(static_cast<ast::UnaryExpr*>(expr));
    case ast::NodeKind::kArrayType:
      return EvaluateArrayType(static_cast<ast::ArrayType*>(expr));
    case ast::NodeKind::kFuncType:
      return EvaluateFuncType(static_cast<ast::FuncType*>(expr));
    case ast::NodeKind::kInterfaceType:
      return EvaluateInterfaceType(static_cast<ast::InterfaceType*>(expr));
    case ast::NodeKind::kStructType:
      return EvaluateStructType(static_cast<ast::StructType*>(expr));
    case ast::NodeKind::kTypeInstance:
      return EvaluateTypeInstance(static_cast<ast::TypeInstance*>(expr));
    default:
      issues().Add(issues::kForbiddenTypeExpression, expr->start(), "type expression not allowed");
      return nullptr;
  }
}

types::Type* TypeHandler::EvaluateTypeIdent(ast::Ident* ident) {
  types::Object* used = info()->uses().at(ident);
  if (used->object_kind() != types::ObjectKind::kTypeName) {
    issues().Add(issues::kObjectIsNotTypeName, ident->start(), "expected type name");
    return nullptr;
  }
  types::TypeName* type_name = static_cast<types::TypeName*>(used);
  info_builder().SetExprInfo(ident,
                             types::ExprInfo(types::ExprInfo::Kind::kType, type_name->type()));
  return type_name->type();
}

types::Pointer* TypeHandler::EvaluatePointerType(ast::UnaryExpr* pointer_expr) {
  types::Pointer::Kind kind;
  switch (pointer_expr->op()) {
    case tokens::kMul:
      kind = types::Pointer::Kind::kStrong;
      break;
    case tokens::kRem:
      kind = types::Pointer::Kind::kWeak;
      break;
    default:
      issues().Add(issues::kUnexpectedPointerPrefix, pointer_expr->start(),
                   "expected '*' or '%' as pointer prefix");
      return nullptr;
  }
  types::Type* element_type = EvaluateTypeExpr(pointer_expr->x());
  if (element_type == nullptr) {
    return nullptr;
  }
  types::Pointer* pointer_type = info_builder().CreatePointer(kind, element_type);
  info_builder().SetExprInfo(pointer_expr,
                             types::ExprInfo(types::ExprInfo::Kind::kType, pointer_type));
  return pointer_type;
}

types::Container* TypeHandler::EvaluateArrayType(ast::ArrayType* array_expr) {
  bool is_slice = (array_expr->len() == nullptr);
  uint64_t length = -1;
  if (!is_slice) {
    if (!type_resolver().expr_handler().CheckExpr(
            array_expr->len(), ExprHandler::Context(/*expect_constant=*/true, /* iota= */ 0))) {
      issues().Add(issues::kConstantForArraySizeCanNotBeEvaluated, array_expr->len()->start(),
                   "can not evaluate constant for array size");
      return nullptr;
    }
    constants::Value length_value = info()->ExprInfoOf(array_expr->len()).value().constant_value();
    if (!length_value.CanConvertToArraySize()) {
      issues().Add(issues::kConstantCanNotBeUsedAsArraySize, array_expr->len()->start(),
                   "can not use constant as array size");
      return nullptr;
    }
    length = length_value.ConvertToArraySize();
  }
  types::Type* element_type = EvaluateTypeExpr(array_expr->element_type());
  if (element_type == nullptr) {
    return nullptr;
  }

  if (!is_slice) {
    types::Array* array_type = info_builder().CreateArray(element_type, length);
    info_builder().SetExprInfo(array_expr,
                               types::ExprInfo(types::ExprInfo::Kind::kType, array_type));
    return array_type;

  } else {
    types::Slice* slice_type = info_builder().CreateSlice(element_type);
    info_builder().SetExprInfo(array_expr,
                               types::ExprInfo(types::ExprInfo::Kind::kType, slice_type));
    return slice_type;
  }
}

types::Signature* TypeHandler::EvaluateFuncType(ast::FuncType* func_expr) {
  types::Tuple* parameters = EvaluateTuple(func_expr->params());
  if (parameters == nullptr) {
    return nullptr;
  }
  types::Tuple* results = nullptr;
  if (func_expr->results() != nullptr) {
    results = EvaluateTuple(func_expr->results());
    if (results == nullptr) {
      return nullptr;
    }
  }
  types::Signature* signature = info_builder().CreateSignature(parameters, results);
  info_builder().SetExprInfo(func_expr, types::ExprInfo(types::ExprInfo::Kind::kType, signature));
  return signature;
}

types::Interface* TypeHandler::EvaluateInterfaceType(ast::InterfaceType* interface_expr) {
  types::Interface* interface_type = info_builder().CreateInterface();

  std::vector<types::Func*> methods;
  methods.reserve(interface_expr->methods().size());
  for (ast::MethodSpec* method_spec : interface_expr->methods()) {
    types::Func* method = EvaluateMethodSpec(method_spec, interface_type);
    if (method == nullptr) {
      return nullptr;
    }
    methods.push_back(method);
  }
  // TODO: handle embdedded interfaces
  info_builder().SetInterfaceMembers(interface_type, {}, methods);
  info_builder().SetExprInfo(interface_expr,
                             types::ExprInfo(types::ExprInfo::Kind::kType, interface_type));
  return interface_type;
}

types::Struct* TypeHandler::EvaluateStructType(ast::StructType* struct_expr) {
  std::vector<types::Variable*> fields = EvaluateFieldList(struct_expr->fields());
  if (fields.empty()) {
    // TODO: handle empty struct
    return nullptr;
  }
  types::Struct* struct_type = info_builder().CreateStruct(fields);
  info_builder().SetExprInfo(struct_expr,
                             types::ExprInfo(types::ExprInfo::Kind::kType, struct_type));
  return struct_type;
}

types::TypeInstance* TypeHandler::EvaluateTypeInstance(ast::TypeInstance* type_instance_expr) {
  types::Type* instantiated_type = EvaluateTypeExpr(type_instance_expr->type());
  if (instantiated_type == nullptr ||
      instantiated_type->type_kind() != types::TypeKind::kNamedType) {
    return nullptr;
  }
  types::NamedType* instantiated_named_type = static_cast<types::NamedType*>(instantiated_type);
  if (type_instance_expr->type_args().size() != instantiated_named_type->type_parameters().size()) {
    issues().Add(issues::kWrongNumberOfTypeArgumentsForTypeInstance, type_instance_expr->l_brack(),
                 "type instance has wrong number of type arguments");
    return nullptr;
  }

  std::vector<types::Type*> type_args;
  type_args.reserve(instantiated_named_type->type_parameters().size());
  for (size_t i = 0; i < instantiated_named_type->type_parameters().size(); i++) {
    types::TypeParameter* type_param = instantiated_named_type->type_parameters().at(i);
    ast::Expr* type_arg_expr = type_instance_expr->type_args().at(i);
    types::Type* type_arg = EvaluateTypeExpr(type_arg_expr);
    if (type_arg == nullptr) {
      return nullptr;
    }
    if (!types::IsAssertableTo(type_param, type_arg)) {
      issues().Add(issues::kTypeArgumentCanNotBeUsedForTypeInstanceParameter,
                   type_arg_expr->start(), "type argument can not be used for type parameter");
      return nullptr;
    }

    type_args.push_back(type_arg);
  }

  types::TypeInstance* type_instance =
      info_builder().CreateTypeInstance(instantiated_named_type, type_args);
  info_builder().SetExprInfo(type_instance_expr,
                             types::ExprInfo(types::ExprInfo::Kind::kType, type_instance));
  return type_instance;
}

std::vector<types::TypeParameter*> TypeHandler::EvaluateTypeParameters(
    ast::TypeParamList* parameters_expr) {
  std::vector<types::TypeParameter*> type_parameters;
  type_parameters.reserve(parameters_expr->params().size());
  for (auto& parameter_expr : parameters_expr->params()) {
    types::TypeParameter* type_parameter = EvaluateTypeParameter(parameter_expr);
    if (type_parameter == nullptr) {
      return {};
    }
    type_parameters.push_back(type_parameter);
  }
  return type_parameters;
}

types::TypeParameter* TypeHandler::EvaluateTypeParameter(ast::TypeParam* parameter_expr) {
  types::Interface* interface = nullptr;
  if (parameter_expr->type() != nullptr) {
    types::Type* type = EvaluateTypeExpr(parameter_expr->type());
    if (type == nullptr) {
      return nullptr;
    }
    types::Type* underlying = types::UnderlyingOf(type, info_builder());
    if (underlying->type_kind() != types::TypeKind::kInterface) {
      issues().Add(issues::kTypeParamterConstraintIsNotInterface, parameter_expr->type()->start(),
                   "type parameter constraint has to be an interface");
      return nullptr;
    }
    interface = static_cast<types::Interface*>(underlying);
  } else {
    interface = info_builder().CreateInterface();
  }

  auto type_name = static_cast<types::TypeName*>(info()->DefinitionOf(parameter_expr->name()));
  auto type_parameter = static_cast<types::TypeParameter*>(type_name->type());
  info_builder().SetTypeParameterInterface(type_parameter, interface);
  return type_parameter;
}

types::Func* TypeHandler::EvaluateMethodSpec(ast::MethodSpec* method_spec,
                                             types::Interface* interface) {
  types::TypeParameter* instance_type = nullptr;
  if (method_spec->instance_type_param() != nullptr) {
    types::TypeName* instance_type_param =
        static_cast<types::TypeName*>(info()->DefinitionOf(method_spec->instance_type_param()));
    instance_type = static_cast<types::TypeParameter*>(instance_type_param->type());
    info_builder().SetTypeParameterInterface(instance_type, interface);
  }

  types::Tuple* parameters = EvaluateTuple(method_spec->params());
  if (parameters == nullptr) {
    return nullptr;
  }
  types::Tuple* results = nullptr;
  if (method_spec->results() != nullptr) {
    results = EvaluateTuple(method_spec->results());
    if (results == nullptr) {
      return nullptr;
    }
  }
  ast::Ident* name = method_spec->name();
  types::Func* func = static_cast<types::Func*>(info()->DefinitionOf(name));
  types::Signature* signature = info_builder().CreateSignature(instance_type, parameters, results);
  info_builder().SetObjectType(func, signature);
  return func;
}

types::Tuple* TypeHandler::EvaluateTuple(ast::FieldList* field_list) {
  std::vector<types::Variable*> variables = EvaluateFieldList(field_list);
  if (variables.empty() && !field_list->fields().empty()) {
    return nullptr;
  }
  return info_builder().CreateTuple(variables);
}

std::vector<types::Variable*> TypeHandler::EvaluateFieldList(ast::FieldList* field_list) {
  std::vector<types::Variable*> variables;
  for (ast::Field* field : field_list->fields()) {
    std::vector<types::Variable*> field_vars = EvaluateField(field);
    if (field_vars.empty()) {
      return {};
    }
    variables.reserve(variables.size() + field_vars.size());
    variables.insert(variables.end(), field_vars.begin(), field_vars.end());
  }
  return variables;
}

std::vector<types::Variable*> TypeHandler::EvaluateField(ast::Field* field) {
  types::Type* field_type = EvaluateTypeExpr(field->type());
  if (field_type == nullptr) {
    return {};
  }

  std::vector<types::Variable*> variables;
  variables.reserve(std::max(size_t{1}, field->names().size()));
  if (!field->names().empty()) {
    for (ast::Ident* name : field->names()) {
      types::Variable* variable = static_cast<types::Variable*>(info()->DefinitionOf(name));
      info_builder().SetObjectType(variable, field_type);
      variables.push_back(variable);
    }

  } else {
    types::Variable* variable = static_cast<types::Variable*>(info()->ImplicitOf(field));
    info_builder().SetObjectType(variable, field_type);
    variables.push_back(variable);
  }
  return variables;
}

}  // namespace type_checker
}  // namespace lang

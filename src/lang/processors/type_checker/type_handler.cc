//
//  type_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "type_handler.h"

#include <algorithm>

#include "lang/representation/types/types_util.h"
#include "lang/processors/type_checker/constant_handler.h"

namespace lang {
namespace type_checker {

bool TypeHandler::ProcessTypeName(types::TypeName *type_name,
                                  ast::TypeSpec *type_spec,
                                  types::InfoBuilder& info_builder,
                                  std::vector<issues::Issue>& issues) {
    TypeHandler handler(info_builder, issues);
    
    if (!handler.ProcessTypeParameters(type_name, type_spec)) {
        return false;
    }
    return handler.ProcessUnderlyingType(type_name, type_spec);
}

bool TypeHandler::ProcessTypeParametersOfTypeName(types::TypeName *type_name,
                                                  ast::TypeSpec *type_spec,
                                                  types::InfoBuilder& info_builder,
                                                  std::vector<issues::Issue>& issues) {
    TypeHandler handler(info_builder, issues);
    
    return handler.ProcessTypeParameters(type_name, type_spec);
}

bool TypeHandler::ProcessUnderlyingTypeOfTypeName(types::TypeName *type_name,
                                                  ast::TypeSpec *type_spec,
                                                  types::InfoBuilder& info_builder,
                                                  std::vector<issues::Issue>& issues) {
    TypeHandler handler(info_builder, issues);
    
    return handler.ProcessUnderlyingType(type_name, type_spec);
}

bool TypeHandler::ProcessFuncDecl(types::Func *func,
                                  ast::FuncDecl *func_decl,
                                  types::InfoBuilder& info_builder,
                                  std::vector<issues::Issue>& issues) {
    TypeHandler handler(info_builder, issues);
    
    return handler.ProcessFuncDefinition(func, func_decl);
}

bool TypeHandler::ProcessTypeArgs(std::vector<ast::Expr *> type_args,
                                  types::InfoBuilder& info_builder,
                                  std::vector<issues::Issue> &issues) {
    TypeHandler handler(info_builder, issues);
    
    bool ok = true;
    for (ast::Expr *arg : type_args) {
        ok = handler.EvaluateTypeExpr(arg) && ok;
    }
    return ok;
}

bool TypeHandler::ProcessTypeExpr(ast::Expr *type_expr,
                                  types::InfoBuilder& info_builder,
                                  std::vector<issues::Issue>& issues) {
    TypeHandler handler(info_builder, issues);
    
    return handler.EvaluateTypeExpr(type_expr);
}

bool TypeHandler::ProcessTypeParameters(types::TypeName *type_name,
                                        ast::TypeSpec *type_spec) {
    std::vector<types::TypeParameter *> type_parameters;
    if (type_spec->type_params() != nullptr) {
        type_parameters = EvaluateTypeParameters(type_spec->type_params());
        if (type_parameters.empty()) {
            return false;
        }
    }
    types::NamedType *named_type = static_cast<types::NamedType *>(type_name->type());
    info_builder_.SetTypeParametersOfNamedType(named_type, type_parameters);
    return true;
}

bool TypeHandler::ProcessUnderlyingType(types::TypeName *type_name,
                                        ast::TypeSpec *type_spec) {
    if (!EvaluateTypeExpr(type_spec->type())) {
        return false;
    }
    types::ExprInfo underling_info = info_->ExprInfoOf(type_spec->type()).value();
    types::Type *underlying_type = underling_info.type();
    types::NamedType *named_type = static_cast<types::NamedType *>(type_name->type());
    info_builder_.SetUnderlyingTypeOfNamedType(named_type, underlying_type);
    return true;
}

bool TypeHandler::ProcessFuncDefinition(types::Func *func,
                                        ast::FuncDecl *func_decl) {
    if (func_decl->kind() != ast::FuncDecl::Kind::kFunc && func_decl->type_params()) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        func_decl->start(),
                                        "method can not declare type parameters"));
    }
    
    types::Variable *expr_receiver = nullptr;
    types::Type *type_receiver = nullptr;
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
    
    std::vector<types::TypeParameter *> type_parameters;
    if (func_decl->type_params() != nullptr) {
        type_parameters = EvaluateTypeParameters(func_decl->type_params());
        if (type_parameters.empty()) {
            return false;
        }
    }
    
    types::Tuple *parameters = EvaluateTuple(func_decl->func_type()->params());
    types::Tuple *results = nullptr;
    if (func_decl->func_type()->results() != nullptr) {
        results = EvaluateTuple(func_decl->func_type()->results());
        if (results == nullptr) {
            return false;
        }
    }
    
    types::Signature *signature;
    if (expr_receiver != nullptr) {
        signature = info_builder_.CreateSignature(expr_receiver,
                                                  parameters,
                                                  results);
    } else if (type_receiver != nullptr) {
        signature = info_builder_.CreateSignature(type_receiver,
                                                  parameters,
                                                  results);
    } else {
        signature = info_builder_.CreateSignature(type_parameters,
                                                  parameters,
                                                  results);
    }
    info_builder_.SetObjectType(func, signature);
    return true;
}

bool TypeHandler::EvaluateTypeExpr(ast::Expr *expr) {
    switch (expr->node_kind()) {
        case ast::NodeKind::kIdent:
            return EvaluateTypeIdent(static_cast<ast::Ident *>(expr));
        case ast::NodeKind::kParenExpr:{
            ast::ParenExpr *paren_expr = static_cast<ast::ParenExpr *>(expr);
            if (!EvaluateTypeExpr(paren_expr->x())) {
                return false;
            }
            types::ExprInfo x_info = info_->ExprInfoOf(paren_expr->x()).value();
            info_builder_.SetExprInfo(expr, x_info);
            return true;
        }
        case ast::NodeKind::kSelectionExpr:{
            ast::SelectionExpr *selector_expr = static_cast<ast::SelectionExpr *>(expr);
            if (selector_expr->accessed()->node_kind() != ast::NodeKind::kIdent) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "type expression not allowed"));
                return false;
            }
            ast::Ident *ident = static_cast<ast::Ident *>(selector_expr->accessed());
            if (info_->uses().at(ident)->object_kind() != types::ObjectKind::kPackageName) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "type expression not allowed"));
                return false;
            }
            return EvaluateTypeIdent(selector_expr->selection());
        }
        case ast::NodeKind::kUnaryExpr:
            return EvaluatePointerType(static_cast<ast::UnaryExpr *>(expr));
        case ast::NodeKind::kArrayType:
            return EvaluateArrayType(static_cast<ast::ArrayType *>(expr));
        case ast::NodeKind::kFuncType:
            return EvaluateFuncType(static_cast<ast::FuncType *>(expr));
        case ast::NodeKind::kInterfaceType:
            return EvaluateInterfaceType(static_cast<ast::InterfaceType *>(expr));
        case ast::NodeKind::kStructType:
            return EvaluateStructType(static_cast<ast::StructType *>(expr));
        case ast::NodeKind::kTypeInstance:
            return EvaluateTypeInstance(static_cast<ast::TypeInstance *>(expr));
        default:
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            expr->start(),
                                            "type expression not allowed"));
            return false;
    }
}

bool TypeHandler::EvaluateTypeIdent(ast::Ident *ident) {
    types::Object *used = info_->uses().at(ident);
    if (used->object_kind() != types::ObjectKind::kTypeName) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        ident->start(),
                                        "expected type name"));
        return false;
    }
    types::TypeName *type_name = static_cast<types::TypeName *>(used);
    info_builder_.SetExprInfo(ident, types::ExprInfo(types::ExprInfo::Kind::kType,
                                                     type_name->type()));
    return true;
}

bool TypeHandler::EvaluatePointerType(ast::UnaryExpr *pointer_expr) {
    types::Pointer::Kind kind;
    switch (pointer_expr->op()) {
        case tokens::kMul:
            kind = types::Pointer::Kind::kStrong;
            break;
        case tokens::kRem:
            kind = types::Pointer::Kind::kWeak;
            break;
        default:
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            pointer_expr->start(),
                                            "expected '*' or '%' as pointer prefix"));
            return false;
    }
    if (!EvaluateTypeExpr(pointer_expr->x())) {
        return false;
    }
    types::ExprInfo element_info = info_->ExprInfoOf(pointer_expr->x()).value();
    types::Pointer *pointer_type = info_builder_.CreatePointer(kind, element_info.type());
    info_builder_.SetExprInfo(pointer_expr, types::ExprInfo(types::ExprInfo::Kind::kType,
                                                            pointer_type));
    return true;
}

bool TypeHandler::EvaluateArrayType(ast::ArrayType *array_expr) {
    bool is_slice = (array_expr->len() == nullptr);
    uint64_t length = -1;
    if (!is_slice) {
        if (!ConstantHandler::ProcessConstantExpr(array_expr->len(),
                                                  /* iota= */0,
                                                  info_builder_,
                                                  issues_)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            array_expr->len()->start(),
                                            "could not evaluate array size"));
            return false;
        }
        constants::Value length_value = info_->constant_values().at(array_expr->len());
        if (!length_value.CanConvertToArraySize()) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            array_expr->len()->start(),
                                            "can not use constant as array size"));
            return false;
        }
        length = length_value.ConvertToArraySize();
    }
    if (!EvaluateTypeExpr(array_expr->element_type())) {
        return false;
    }
    types::ExprInfo element_info = info_->ExprInfoOf(array_expr->element_type()).value();
    types::Type *element_type = element_info.type();
    
    if (!is_slice) {
        types::Array *array_type = info_builder_.CreateArray(element_type, length);
        info_builder_.SetExprInfo(array_expr, types::ExprInfo(types::ExprInfo::Kind::kType,
                                                              array_type));
        return true;
        
    } else {
        types::Slice *slice_type = info_builder_.CreateSlice(element_type);
        info_builder_.SetExprInfo(array_expr, types::ExprInfo(types::ExprInfo::Kind::kType,
                                                              slice_type));
        return true;
    }
}

bool TypeHandler::EvaluateFuncType(ast::FuncType *func_expr) {
    types::Tuple *parameters = EvaluateTuple(func_expr->params());
    if (parameters == nullptr) {
        return false;
    }
    types::Tuple *results = nullptr;
    if (func_expr->results() != nullptr) {
        results = EvaluateTuple(func_expr->results());
        if (results == nullptr) {
            return false;
        }
    }
    types::Signature *signature = info_builder_.CreateSignature(parameters,
                                                                results);
    info_builder_.SetExprInfo(func_expr, types::ExprInfo(types::ExprInfo::Kind::kType,
                                                         signature));
    return true;
}

bool TypeHandler::EvaluateInterfaceType(ast::InterfaceType *interface_expr) {
    types::Interface *interface_type = info_builder_.CreateInterface();

    std::vector<types::Func *> methods;
    methods.reserve(interface_expr->methods().size());
    for (ast::MethodSpec *method_spec : interface_expr->methods()) {
        types::Func *method = EvaluateMethodSpec(method_spec, interface_type);
        if (method == nullptr) {
            return false;
        }
        methods.push_back(method);
    }
    // TODO: handle embdedded interfaces
    info_builder_.SetInterfaceMembers(interface_type, {}, methods);
    info_builder_.SetExprInfo(interface_expr, types::ExprInfo(types::ExprInfo::Kind::kType,
                                                              interface_type));
    return true;
}

bool TypeHandler::EvaluateStructType(ast::StructType *struct_expr) {
    std::vector<types::Variable *> fields =
        EvaluateFieldList(struct_expr->fields());
    if (fields.empty()) {
        return false;
    }
    types::Struct *struct_type = info_builder_.CreateStruct(fields);
    info_builder_.SetExprInfo(struct_expr, types::ExprInfo(types::ExprInfo::Kind::kType,
                                                           struct_type));
    return true;
}

bool TypeHandler::EvaluateTypeInstance(ast::TypeInstance *type_instance_expr) {
    if (!EvaluateTypeExpr(type_instance_expr->type())) {
        return false;
    }
    types::ExprInfo instantiated_type_info = info_->ExprInfoOf(type_instance_expr->type()).value();
    types::NamedType *instantiated_type =
        static_cast<types::NamedType *>(instantiated_type_info.type());
    if (type_instance_expr->type_args().size() != instantiated_type->type_parameters().size()) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_instance_expr->l_brack(),
                                        "type instance has wrong number of type arguments"));
        return false;
    }
    
    std::vector<types::Type *> type_args;
    type_args.reserve(instantiated_type->type_parameters().size());
    for (int i = 0; i < instantiated_type->type_parameters().size(); i++) {
        types::TypeParameter *type_param = instantiated_type->type_parameters().at(i);
        ast::Expr *type_arg_expr = type_instance_expr->type_args().at(i);
        if (!EvaluateTypeExpr(type_arg_expr)) {
            return false;
        }
        types::ExprInfo type_arg_expr_info = info_->ExprInfoOf(type_arg_expr).value();
        types::Type *type_arg = type_arg_expr_info.type();
        if (!types::IsAssertableTo(type_param, type_arg)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            type_arg_expr->start(),
                                            "type argument can not be used for type parameter"));
            return false;
        }
        
        type_args.push_back(type_arg);
    }
    
    types::TypeInstance *type_instance = info_builder_.CreateTypeInstance(instantiated_type,
                                                                          type_args);
    info_builder_.SetExprInfo(type_instance_expr, types::ExprInfo(types::ExprInfo::Kind::kType,
                                                                  type_instance));
    return true;
}

std::vector<types::TypeParameter *>
TypeHandler::EvaluateTypeParameters(ast::TypeParamList *parameters_expr) {
    std::vector<types::TypeParameter *> type_parameters;
    type_parameters.reserve(parameters_expr->params().size());
    for (auto& parameter_expr : parameters_expr->params()) {
        types::TypeParameter *type_parameter = EvaluateTypeParameter(parameter_expr);
        if (type_parameter == nullptr) {
            return {};
        }
        type_parameters.push_back(type_parameter);
    }
    return type_parameters;
}

types::TypeParameter * TypeHandler::EvaluateTypeParameter(ast::TypeParam *parameter_expr) {
    types::Interface *interface = nullptr;
    if (parameter_expr->type() != nullptr) {
        if (!EvaluateTypeExpr(parameter_expr->type())) {
            return nullptr;
        }
        types::ExprInfo type_info = info_->ExprInfoOf(parameter_expr->type()).value();
        types::Type *type = type_info.type();
        types::Type *underlying = types::UnderlyingOf(type);
        if (underlying->type_kind() != types::TypeKind::kInterface) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            parameter_expr->type()->start(),
                                            "type parameter constraint has to be an interface"));
            return nullptr;
        }
        interface = static_cast<types::Interface *>(underlying);
    } else {
        interface = info_builder_.CreateInterface();
    }
    
    auto type_name = static_cast<types::TypeName *>(info_->DefinitionOf(parameter_expr->name()));
    auto type_parameter = static_cast<types::TypeParameter *>(type_name->type());
    info_builder_.SetTypeParameterInterface(type_parameter, interface);
    return type_parameter;
}

types::Func * TypeHandler::EvaluateMethodSpec(ast::MethodSpec *method_spec,
                                              types::Interface *interface) {
    types::TypeParameter *instance_type = nullptr;
    if (method_spec->instance_type_param() != nullptr) {
        types::TypeName *instance_type_param =
            static_cast<types::TypeName *>(info_->DefinitionOf(method_spec->instance_type_param()));
        instance_type = static_cast<types::TypeParameter *>(instance_type_param->type());
        info_builder_.SetTypeParameterInterface(instance_type, interface);
    }
    
    types::Tuple *parameters = EvaluateTuple(method_spec->params());
    if (parameters == nullptr) {
        return nullptr;
    }
    types::Tuple *results = nullptr;
    if (method_spec->results() != nullptr) {
        results = EvaluateTuple(method_spec->results());
        if (results == nullptr) {
            return nullptr;
        }
    }
    ast::Ident *name = method_spec->name();
    types::Func *func = static_cast<types::Func *>(info_->DefinitionOf(name));
    types::Signature *signature = info_builder_.CreateSignature(instance_type,
                                                                parameters,
                                                                results);
    info_builder_.SetObjectType(func, signature);
    return func;
}

types::Tuple * TypeHandler::EvaluateTuple(ast::FieldList *field_list) {
    std::vector<types::Variable *> variables = EvaluateFieldList(field_list);
    if (variables.empty() && !field_list->fields().empty()) {
        return nullptr;
    }
    return info_builder_.CreateTuple(variables);
}

std::vector<types::Variable *> TypeHandler::EvaluateFieldList(ast::FieldList *field_list) {
    std::vector<types::Variable *> variables;
    for (ast::Field *field : field_list->fields()) {
        std::vector<types::Variable *> field_vars = EvaluateField(field);
        if (field_vars.empty()) {
            return {};
        }
        variables.reserve(variables.size() + field_vars.size());
        variables.insert(variables.end(), field_vars.begin(), field_vars.end());
    }
    return variables;
}

std::vector<types::Variable *> TypeHandler::EvaluateField(ast::Field *field) {
    if (!EvaluateTypeExpr(field->type())) {
        return {};
    }
    types::ExprInfo field_type_info = info_->ExprInfoOf(field->type()).value();
    
    std::vector<types::Variable *> variables;
    variables.reserve(std::max(size_t{1}, field->names().size()));
    if (!field->names().empty()) {
        for (ast::Ident *name : field->names()) {
            types::Variable *variable =
                static_cast<types::Variable *>(info_->DefinitionOf(name));
            info_builder_.SetObjectType(variable, field_type_info.type());
            variables.push_back(variable);
        }
        
    } else {
        types::Variable *variable =
            static_cast<types::Variable *>(info_->ImplicitOf(field));
        info_builder_.SetObjectType(variable, field_type_info.type());
        variables.push_back(variable);
    }
    return variables;
}

types::Variable * TypeHandler::EvaluateExprReceiver(ast::ExprReceiver *expr_receiver,
                                                    types::Func *method) {
    types::Type *type = EvalutateReceiverTypeInstance(expr_receiver->type_name(),
                                                      expr_receiver->type_parameter_names(),
                                                      method);
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

        types::Pointer *pointer_type = info_builder_.CreatePointer(kind, type);
        type = pointer_type;
    }
    
    types::Variable *receiver;
    if (expr_receiver->name() != nullptr) {
        receiver = static_cast<types::Variable *>(info_->DefinitionOf(expr_receiver->name()));
    } else {
        receiver = static_cast<types::Variable *>(info_->ImplicitOf(expr_receiver));
    }
    info_builder_.SetObjectType(receiver, type);
    return receiver;
}

types::Type * TypeHandler::EvaluateTypeReceiver(ast::TypeReceiver *type_receiver,
                                                types::Func *method) {
    return EvalutateReceiverTypeInstance(type_receiver->type_name(),
                                         type_receiver->type_parameter_names(),
                                         method);
}

types::Type *
TypeHandler::EvalutateReceiverTypeInstance(ast::Ident *type_name_ident,
                                           std::vector<ast::Ident *> type_param_names,
                                           types::Func *method) {
    types::TypeName *type_name = static_cast<types::TypeName *>(info_->UseOf(type_name_ident));
    if (type_name->type()->type_kind() != types::TypeKind::kNamedType) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_name_ident->start(),
                                        "receiver does not have named type"));
        return nullptr;
    }
    types::NamedType *named_type = static_cast<types::NamedType *>(type_name->type());
    if (named_type->underlying()->type_kind() == types::TypeKind::kInterface) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_name_ident->start(),
                                        "can not define additional methods for interfaces"));
        return nullptr;
    } else if (named_type->methods().contains(method->name())) {
        types::Func *other_method = named_type->methods().at(method->name());
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        {other_method->position(), method->position()},
                                        "can not define two methods with the same name"));
        return nullptr;
    }
    info_builder_.AddMethodToNamedType(named_type, method);
    
    if (type_param_names.size() != named_type->type_parameters().size()) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_name_ident->start(),
                                        "receiver has wrong number of type arguments"));
        return nullptr;
    }
    if (!named_type->type_parameters().empty()) {
        std::vector<types::Type *> type_instance_args;
        type_instance_args.reserve(named_type->type_parameters().size());
        for (int i = 0; i < named_type->type_parameters().size(); i++) {
            types::TypeParameter *instantiated = named_type->type_parameters().at(i);
            ast::Ident *arg_name = type_param_names.at(i);
            types::TypeName *arg = static_cast<types::TypeName *>(info_->DefinitionOf(arg_name));
            types::TypeParameter *instance = static_cast<types::TypeParameter *>(arg->type());
            info_builder_.SetTypeParameterInstance(instantiated, instance);
            type_instance_args.push_back(instance);
        }
        return info_builder_.CreateTypeInstance(named_type,
                                                type_instance_args);
    }
    return named_type;
}

}
}

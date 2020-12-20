//
//  type_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "type_handler.h"

#include <algorithm>

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
    types::Type *underlying_type = info_->TypeOf(type_spec->type());
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
    if (ast::ParenExpr *paren_expr = dynamic_cast<ast::ParenExpr *>(expr)) {
        if (!EvaluateTypeExpr(paren_expr->x())) {
            return false;
        }
        types::Type *type = info_->types().at(paren_expr->x());
        info_builder_.SetExprType(expr, type);
        return true;
        
    } else if (ast::Ident *ident = dynamic_cast<ast::Ident *>(expr)) {
        return EvaluateTypeIdent(ident);
        
    } else if (ast::SelectionExpr *selector_expr = dynamic_cast<ast::SelectionExpr *>(expr)) {
        auto ident = dynamic_cast<ast::Ident *>(selector_expr->accessed());
        if (ident == nullptr) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            expr->start(),
                                            "type expression not allowed"));
            return false;
        }
        auto package = dynamic_cast<types::PackageName *>(info_->uses().at(ident));
        if (package == nullptr) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            expr->start(),
                                            "type expression not allowed"));
            return false;
        }
        return EvaluateTypeIdent(selector_expr->selection());
        
    } else if (ast::UnaryExpr *pointer_type = dynamic_cast<ast::UnaryExpr *>(expr)) {
        return EvaluatePointerType(pointer_type);
        
    } else if (ast::ArrayType *array_type = dynamic_cast<ast::ArrayType *>(expr)) {
        return EvaluateArrayType(array_type);
        
    } else if (ast::FuncType * func_type = dynamic_cast<ast::FuncType *>(expr)) {
        return EvaluateFuncType(func_type);
        
    } else if (ast::InterfaceType * interface_type = dynamic_cast<ast::InterfaceType *>(expr)) {
        return EvaluateInterfaceType(interface_type);
        
    } else if (ast::StructType *struct_type = dynamic_cast<ast::StructType *>(expr)) {
        return EvaluateStructType(struct_type);
        
    } else if (ast::TypeInstance *type_instance = dynamic_cast<ast::TypeInstance *>(expr)) {
        return EvaluateTypeInstance(type_instance);
        
    } else {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        expr->start(),
                                        "type expression not allowed"));
        return false;
    }
}

bool TypeHandler::EvaluateTypeIdent(ast::Ident *ident) {
    types::Object *used = info_->uses().at(ident);
    types::TypeName *type_name = dynamic_cast<types::TypeName *>(used);
    if (type_name == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        ident->start(),
                                        "expected type name"));
        return false;
    }
    info_builder_.SetExprType(ident, type_name->type());
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
    types::Type *element_type = info_->TypeOf(pointer_expr->x());
    types::Pointer *pointer_type = info_builder_.CreatePointer(kind, element_type);
    info_builder_.SetExprType(pointer_expr, pointer_type);
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
    types::Type *element_type = info_->TypeOf(array_expr->element_type());
    
    if (!is_slice) {
        types::Array *array_type = info_builder_.CreateArray(element_type, length);
        info_builder_.SetExprType(array_expr, array_type);
        return true;
        
    } else {
        types::Slice *slice_type = info_builder_.CreateSlice(element_type);
        info_builder_.SetExprType(array_expr, slice_type);
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
    info_builder_.SetExprType(func_expr, signature);
    return true;
}

bool TypeHandler::EvaluateInterfaceType(ast::InterfaceType *interface_expr) {
    types::Interface *interface_type =
    info_builder_.CreateInterface(/* embedded_interfaces= */ {});

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
    info_builder_.SetMethodsOfInterface(interface_type, methods);
    info_builder_.SetExprType(interface_expr, interface_type);
    return true;
}

bool TypeHandler::EvaluateStructType(ast::StructType *struct_expr) {
    std::vector<types::Variable *> fields =
        EvaluateFieldList(struct_expr->fields());
    if (fields.empty()) {
        return false;
    }
    types::Struct *struct_type = info_builder_.CreateStruct(fields);
    info_builder_.SetExprType(struct_expr, struct_type);
    return true;
}

bool TypeHandler::EvaluateTypeInstance(ast::TypeInstance *type_instance_expr) {
    if (!EvaluateTypeExpr(type_instance_expr->type())) {
        return false;
    }
    types::NamedType *instantiated_type =
        static_cast<types::NamedType *>(info_->TypeOf(type_instance_expr->type()));
    
    std::vector<types::Type *> type_args;
    type_args.reserve(type_instance_expr->type_args().size());
    for (ast::Expr *type_arg_expr : type_instance_expr->type_args()) {
        if (!EvaluateTypeExpr(type_arg_expr)) {
            return false;
        }
        types::Type *type_arg = info_->TypeOf(type_arg_expr);
        type_args.push_back(type_arg);
    }
    
    types::TypeInstance *type_instance = info_builder_.CreateTypeInstance(instantiated_type,
                                                                          type_args);
    info_builder_.SetExprType(type_instance_expr, type_instance);
    return true;
}

std::vector<types::TypeParameter *> TypeHandler::EvaluateTypeParameters(ast::TypeParamList *parameters_expr) {
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
    types::Type *interface = nullptr;
    if (parameter_expr->type() != nullptr) {
        if (!EvaluateTypeExpr(parameter_expr->type())) {
            return nullptr;
        }
        interface = info_->TypeOf(parameter_expr->type());
        
        types::Interface *underlying = dynamic_cast<types::Interface *>(interface->Underlying());
        if (underlying == nullptr) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            parameter_expr->type()->start(),
                                            "type parameter constraint has to be an interface"));
            return nullptr;
        }
    } else {
        interface = info_builder_.CreateInterface(/* embedded_interfaces= */ {});
    }
    
    types::TypeName *type_name =
        static_cast<types::TypeName *>(info_->DefinitionOf(parameter_expr->name()));
    
    types::TypeParameter *type_parameter = static_cast<types::TypeParameter *>(type_name->type());
    info_builder_.SetInterfaceOfTypeParameter(type_parameter, interface);
    return type_parameter;
}

types::Func * TypeHandler::EvaluateMethodSpec(ast::MethodSpec *method_spec,
                                              types::Interface *interface) {
    types::TypeParameter *instance_type = nullptr;
    if (method_spec->instance_type_param() != nullptr) {
        types::TypeName *instance_type_param =
            static_cast<types::TypeName *>(info_->DefinitionOf(method_spec->instance_type_param()));
        instance_type = static_cast<types::TypeParameter *>(instance_type_param->type());
        info_builder_.SetInterfaceOfTypeParameter(instance_type, interface);
    }
    
    types::Tuple *parameters = EvaluateTuple(method_spec->params());
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
    if (variables.empty()) {
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
    types::Type *type = info_->TypeOf(field->type());
    
    std::vector<types::Variable *> variables;
    variables.reserve(std::max(size_t{1}, field->names().size()));
    if (!field->names().empty()) {
        for (ast::Ident *name : field->names()) {
            types::Variable *variable =
                static_cast<types::Variable *>(info_->DefinitionOf(name));
            info_builder_.SetObjectType(variable, type);
            variables.push_back(variable);
        }
        
    } else {
        types::Variable *variable =
            static_cast<types::Variable *>(info_->ImplicitOf(field));
        info_builder_.SetObjectType(variable, type);
        variables.push_back(variable);
    }
    return variables;
}

types::Variable * TypeHandler::EvaluateExprReceiver(ast::ExprReceiver *expr_receiver,
                                                    types::Func *method) {
    types::Type *type = info_->UseOf(expr_receiver->type_name())->type();
    types::NamedType *named_type = dynamic_cast<types::NamedType *>(type);
    if (named_type == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        expr_receiver->type_name()->start(),
                                        "receiver does not have named type"));
        return nullptr;
    } else if (nullptr != dynamic_cast<types::Interface *>(named_type->type())) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        expr_receiver->type_name()->start(),
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
    
    if (expr_receiver->type_parameter_names().size() != named_type->type_parameters().size()) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        expr_receiver->type_name()->start(),
                                        "receiver has wrong number of type arguments"));
        return nullptr;
    }
    if (!named_type->type_parameters().empty()) {
        std::vector<types::Type *> type_instance_args;
        type_instance_args.reserve(named_type->type_parameters().size());
        for (int i = 0; i < named_type->type_parameters().size(); i++) {
            types::Type *interface = named_type->type_parameters().at(i)->interface();
            ast::Ident *arg_name = expr_receiver->type_parameter_names().at(i);
            types::TypeName *arg = static_cast<types::TypeName *>(info_->DefinitionOf(arg_name));
            types::TypeParameter *arg_type = static_cast<types::TypeParameter *>(arg->type());
            info_builder_.SetInterfaceOfTypeParameter(arg_type, interface);
            type_instance_args.push_back(arg_type);
        }
        
        types::TypeInstance *type_instance = info_builder_.CreateTypeInstance(named_type,
                                                                              type_instance_args);
        type = type_instance;
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
    types::Type *type = info_->UseOf(type_receiver->type_name())->type();
    types::NamedType *named_type = dynamic_cast<types::NamedType *>(type);
    if (named_type == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_receiver->type_name()->start(),
                                        "receiver does not have named type"));
        return nullptr;
    } else if (nullptr != dynamic_cast<types::Interface *>(named_type->type())) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_receiver->type_name()->start(),
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
    
    if (type_receiver->type_parameter_names().size() != named_type->type_parameters().size()) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_receiver->type_name()->start(),
                                        "receiver has wrong number of type arguments"));
        return nullptr;
    }
    if (!named_type->type_parameters().empty()) {
        std::vector<types::Type *> type_instance_args;
        type_instance_args.reserve(named_type->type_parameters().size());
        for (int i = 0; i < named_type->type_parameters().size(); i++) {
            types::Type *interface = named_type->type_parameters().at(i)->interface();
            ast::Ident *arg_name = type_receiver->type_parameter_names().at(i);
            types::TypeName *arg = static_cast<types::TypeName *>(info_->DefinitionOf(arg_name));
            types::TypeParameter *arg_type = static_cast<types::TypeParameter *>(arg->type());
            info_builder_.SetInterfaceOfTypeParameter(arg_type, interface);
            type_instance_args.push_back(arg_type);
        }
        
        types::TypeInstance *type_instance = info_builder_.CreateTypeInstance(named_type,
                                                                              type_instance_args);
        type = type_instance;
    }
    
    return type;
}

}
}

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

bool TypeHandler::ProcessTypeArgs(ast::TypeArgList *type_args,
                                  types::InfoBuilder& info_builder,
                                  std::vector<issues::Issue> &issues) {
    TypeHandler handler(info_builder, issues);
    
    bool ok = true;
    for (auto& arg : type_args->args_) {
        ok = handler.EvaluateTypeExpr(arg.get()) && ok;
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
    if (type_spec->type_params_) {
        type_parameters = EvaluateTypeParameters(type_spec->type_params_.get());
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
    if (!EvaluateTypeExpr(type_spec->type_.get())) {
        return false;
    }
    types::Type *underlying_type = info_->TypeOf(type_spec->type_.get());
    types::NamedType *named_type = static_cast<types::NamedType *>(type_name->type());
    info_builder_.SetUnderlyingTypeOfNamedType(named_type, underlying_type);
    return true;
}

bool TypeHandler::ProcessFuncDefinition(types::Func *func,
                                        ast::FuncDecl *func_decl) {
    if (func_decl->receiver_ && func_decl->type_params_) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        func_decl->start(),
                                        "methods can not declare type parameters"));
    }
    
    types::Variable *receiver = nullptr;
    if (func_decl->receiver_) {
        receiver = EvaluateReceiver(func_decl->receiver_.get());
        if (receiver == nullptr) {
            return false;
        }
    }
    
    std::vector<types::TypeParameter *> type_parameters;
    if (func_decl->type_params_) {
        type_parameters = EvaluateTypeParameters(func_decl->type_params_.get());
        if (type_parameters.empty()) {
            return false;
        }
    }
    
    types::Tuple *parameters = EvaluateTuple(func_decl->type_->params_.get());
    types::Tuple *results = nullptr;
    if (func_decl->type_->results_) {
        results = EvaluateTuple(func_decl->type_->results_.get());
        if (results == nullptr) {
            return false;
        }
    }
    
    types::Signature *signature = info_builder_.CreateSignature(receiver,
                                                                type_parameters,
                                                                parameters,
                                                                results);
    info_builder_.SetObjectType(func, signature);
    return true;
}

bool TypeHandler::EvaluateTypeExpr(ast::Expr *expr) {
    if (ast::ParenExpr *paren_expr = dynamic_cast<ast::ParenExpr *>(expr)) {
        if (!EvaluateTypeExpr(paren_expr->x_.get())) {
            return false;
        }
        types::Type *type = info_->types().at(paren_expr->x_.get());
        info_builder_.SetExprType(expr, type);
        return true;
        
    } else if (ast::Ident *ident = dynamic_cast<ast::Ident *>(expr)) {
        return EvaluateTypeIdent(ident);
        
    } else if (ast::SelectionExpr *selector_expr = dynamic_cast<ast::SelectionExpr *>(expr)) {
        auto ident = dynamic_cast<ast::Ident *>(selector_expr->accessed_.get());
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
        return EvaluateTypeIdent(selector_expr->selection_.get());
        
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
    switch (pointer_expr->op_) {
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
    if (!EvaluateTypeExpr(pointer_expr->x_.get())) {
        return false;
    }
    types::Type *element_type = info_->TypeOf(pointer_expr->x_.get());
    types::Pointer *pointer_type = info_builder_.CreatePointer(kind, element_type);
    info_builder_.SetExprType(pointer_expr, pointer_type);
    return true;
}

bool TypeHandler::EvaluateArrayType(ast::ArrayType *array_expr) {
    bool is_slice = (array_expr->len_ == nullptr);
    uint64_t length = -1;
    if (!is_slice) {
        if (!ConstantHandler::ProcessConstantExpr(array_expr->len_.get(),
                                                  /* iota= */0,
                                                  info_builder_,
                                                  issues_)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            array_expr->len_->start(),
                                            "could not evaluate array size"));
            return false;
        }
        constants::Value length_value = info_->constant_values().at(array_expr->len_.get());
        if (!length_value.CanConvertToArraySize()) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            array_expr->len_->start(),
                                            "can not use constant as array size"));
            return false;
        }
        length = length_value.ConvertToArraySize();
    }
    if (!EvaluateTypeExpr(array_expr->element_type_.get())) {
        return false;
    }
    types::Type *element_type = info_->TypeOf(array_expr->element_type_.get());
    
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
    types::Tuple *parameters = EvaluateTuple(func_expr->params_.get());
    if (parameters == nullptr) {
        return false;
    }
    types::Tuple *results = nullptr;
    if (func_expr->results_) {
        results = EvaluateTuple(func_expr->results_.get());
        if (results == nullptr) {
            return false;
        }
    }
    types::Signature *signature = info_builder_.CreateSignature(/* receiver= */ nullptr,
                                                                /* type_parameters= */ {},
                                                                parameters,
                                                                results);
    info_builder_.SetExprType(func_expr, signature);
    return true;
}

bool TypeHandler::EvaluateInterfaceType(ast::InterfaceType *interface_expr) {
    std::vector<types::Func *> methods;
    methods.reserve(interface_expr->methods_.size());
    for (auto& method_spec : interface_expr->methods_) {
        types::Func *method = EvaluateMethodSpec(method_spec.get());
        if (method == nullptr) {
            return false;
        }
        methods.push_back(method);
    }
    types::Interface *interface_type =
        info_builder_.CreateInterface(/* embedded_interfaces= */ {}, methods);
    info_builder_.SetExprType(interface_expr, interface_type);
    return true;
}

bool TypeHandler::EvaluateStructType(ast::StructType *struct_expr) {
    std::vector<types::Variable *> fields =
        EvaluateFieldList(struct_expr->fields_.get());
    if (fields.empty()) {
        return false;
    }
    types::Struct *struct_type = info_builder_.CreateStruct(fields);
    info_builder_.SetExprType(struct_expr, struct_type);
    return true;
}

bool TypeHandler::EvaluateTypeInstance(ast::TypeInstance *type_instance_expr) {
    if (!EvaluateTypeExpr(type_instance_expr->type_.get())) {
        return false;
    }
    types::NamedType *instantiated_type =
        static_cast<types::NamedType *>(info_->TypeOf(type_instance_expr->type_.get()));
    
    std::vector<types::Type *> type_args;
    type_args.reserve(type_instance_expr->type_args_->args_.size());
    for (auto& type_arg_expr : type_instance_expr->type_args_->args_) {
        if (!EvaluateTypeExpr(type_arg_expr.get())) {
            return false;
        }
        types::Type *type_arg = info_->TypeOf(type_arg_expr.get());
        type_args.push_back(type_arg);
    }
    
    types::TypeInstance *type_instance = info_builder_.CreateTypeInstance(instantiated_type,
                                                                          type_args);
    info_builder_.SetExprType(type_instance_expr, type_instance);
    return true;
}

std::vector<types::TypeParameter *> TypeHandler::EvaluateTypeParameters(ast::TypeParamList *parameters_expr) {
    std::vector<types::TypeParameter *> type_parameters;
    type_parameters.reserve(parameters_expr->params_.size());
    for (auto& parameter_expr : parameters_expr->params_) {
        types::TypeParameter *type_parameter = EvaluateTypeParameter(parameter_expr.get());
        if (type_parameter == nullptr) {
            return {};
        }
        type_parameters.push_back(type_parameter);
    }
    return type_parameters;
}

types::TypeParameter * TypeHandler::EvaluateTypeParameter(ast::TypeParam *parameter_expr) {
    types::Type *interface = nullptr;
    if (parameter_expr->type_) {
        if (!EvaluateTypeExpr(parameter_expr->type_.get())) {
            return nullptr;
        }
        interface = info_->TypeOf(parameter_expr->type_.get());
        
        types::Interface *underlying = dynamic_cast<types::Interface *>(interface->Underlying());
        if (underlying == nullptr) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            parameter_expr->type_->start(),
                                            "type parameter constraint has to be an interface"));
            return nullptr;
        }
    } else {
        interface = info_builder_.CreateInterface(/* embedded_interfaces= */ {},
                                                  /* methods= */ {});
    }
    
    types::TypeName *type_name =
        static_cast<types::TypeName *>(info_->DefinitionOf(parameter_expr->name_.get()));
    
    types::TypeParameter *type_parameter = static_cast<types::TypeParameter *>(type_name->type());
    info_builder_.SetInterfaceOfTypeParameter(type_parameter, interface);
    return type_parameter;
}

types::Func * TypeHandler::EvaluateMethodSpec(ast::MethodSpec *method_spec) {
    types::Tuple *parameters = EvaluateTuple(method_spec->params_.get());
    types::Tuple *results = nullptr;
    if (method_spec->results_) {
        results = EvaluateTuple(method_spec->results_.get());
        if (results == nullptr) {
            return nullptr;
        }
    }
    ast::Ident *name = method_spec->name_.get();
    types::Func *func = static_cast<types::Func *>(info_->DefinitionOf(name));
    // TODO: set receiver to enclosing interface
    types::Signature *signature = info_builder_.CreateSignature(/* receiver= */ nullptr,
                                                                /* type_parameters= */ {},
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
    for (auto& field : field_list->fields_) {
        std::vector<types::Variable *> field_vars = EvaluateField(field.get());
        if (field_vars.empty()) {
            return {};
        }
        variables.reserve(variables.size() + field_vars.size());
        variables.insert(variables.end(), field_vars.begin(), field_vars.end());
    }
    return variables;
}

std::vector<types::Variable *> TypeHandler::EvaluateField(ast::Field *field) {
    if (!EvaluateTypeExpr(field->type_.get())) {
        return {};
    }
    types::Type *type = info_->TypeOf(field->type_.get());
    
    std::vector<types::Variable *> variables;
    variables.reserve(std::max(size_t{1}, field->names_.size()));
    if (!field->names_.empty()) {
        for (auto& name : field->names_) {
            types::Variable *variable =
                static_cast<types::Variable *>(info_->DefinitionOf(name.get()));
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

types::Variable * TypeHandler::EvaluateReceiver(ast::FieldList *receiver_list_expr) {
    ast::Field *receiver_expr = receiver_list_expr->fields_.at(0).get();
    ast::Ident *name = receiver_expr->names_.at(0).get();
    ast::Expr *type_expr = receiver_expr->type_.get();
    
    ast::UnaryExpr *pointer_type_expr = dynamic_cast<ast::UnaryExpr *>(type_expr);
    ast::TypeInstance *type_instance_expr = nullptr;
    ast::Ident *type_name = nullptr;
    if (pointer_type_expr != nullptr) {
        type_instance_expr = dynamic_cast<ast::TypeInstance *>(pointer_type_expr->x_.get());
    } else {
        type_instance_expr = dynamic_cast<ast::TypeInstance *>(type_expr);
    }
    if (type_instance_expr != nullptr) {
        type_name = static_cast<ast::Ident *>(type_instance_expr->type_.get());
    } else if (pointer_type_expr != nullptr) {
        type_name = static_cast<ast::Ident *>(pointer_type_expr->x_.get());
    } else {
        type_name = static_cast<ast::Ident *>(type_name);
    }
        
    types::Type *type = info_->UseOf(type_name)->type();
    types::NamedType *named_type = dynamic_cast<types::NamedType *>(type);
    if (named_type == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_name->start(),
                                        "receiver does not have named type"));
        return nullptr;
    } else if (nullptr != dynamic_cast<types::Interface *>(named_type->type())) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_name->start(),
                                        "can not define additional methods for interfaces"));
        return nullptr;
    }
    
    if (type_instance_expr != nullptr) {
        types::NamedType *instantiated_type = static_cast<types::NamedType *>(type);
        
        if (instantiated_type->type_parameters().size() !=
            type_instance_expr->type_args_->args_.size()) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            type_instance_expr->start(),
                                            "receiver has wrong number of type arguments"));
            return nullptr;
        }
        
        std::vector<types::Type *> type_instance_args;
        type_instance_args.reserve(type_instance_expr->type_args_->args_.size());
        for (int i = 0; i < type_instance_expr->type_args_->args_.size(); i++) {
            types::Type *type_arg_type =
                instantiated_type->type_parameters().at(i)->interface();
            ast::Ident *type_arg_name =
                static_cast<ast::Ident *>(type_instance_expr->type_args_->args_.at(i).get());
            
            types::TypeName *type_arg =
                static_cast<types::TypeName *>(info_->DefinitionOf(type_arg_name));
            info_builder_.SetObjectType(type_arg, type_arg_type);
            type_instance_args.push_back(type_arg_type);
        }
        
        types::TypeInstance *type_instance = info_builder_.CreateTypeInstance(instantiated_type,
                                                                              type_instance_args);
        info_builder_.SetExprType(type_instance_expr, type_instance);
        type = type_instance;
    }
    
    if (pointer_type_expr != nullptr) {
        types::Pointer::Kind kind;
        switch (pointer_type_expr->op_) {
            case tokens::kMul:
                kind = types::Pointer::Kind::kStrong;
                break;
            case tokens::kRem:
                kind = types::Pointer::Kind::kWeak;
            default:
                throw "unexpected pointer type";
        }

        types::Pointer *pointer_type = info_builder_.CreatePointer(kind, type);
        info_builder_.SetExprType(pointer_type_expr, pointer_type);
        type = pointer_type;
    }
    
    types::Variable *receiver = static_cast<types::Variable *>(info_->DefinitionOf(name));
    info_builder_.SetObjectType(receiver, type);
    return receiver;
}

}
}

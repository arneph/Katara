//
//  expr_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "expr_handler.h"

#include "lang/representation/ast/ast_util.h"
#include "lang/representation/types/types_util.h"
#include "lang/processors/type_checker/type_handler.h"
#include "lang/processors/type_checker/constant_handler.h"
#include "lang/processors/type_checker/stmt_handler.h"

namespace lang {
namespace type_checker {

bool ExprHandler::ProcessExpr(ast::Expr *expr,
                              types::InfoBuilder& info_builder,
                              std::vector<issues::Issue>& issues) {
    ExprHandler handler(info_builder, issues);
    
    return handler.CheckExpr(expr);
}

bool ExprHandler::CheckExprs(std::vector<ast::Expr *> exprs) {
    bool ok = true;
    for (ast::Expr *expr : exprs) {
        ok = CheckExpr(expr) && ok; // Order needed to avoid short circuiting.
    }
    return ok;
}

bool ExprHandler::CheckExpr(ast::Expr *expr) {
    if (ast::UnaryExpr *unary_expr = dynamic_cast<ast::UnaryExpr *>(expr)) {
        switch (unary_expr->op()) {
            case tokens::kAdd:
            case tokens::kSub:
            case tokens::kXor:
                return CheckUnaryArithmeticOrBitExpr(unary_expr);
            case tokens::kNot:
                return CheckUnaryLogicExpr(unary_expr);
            case tokens::kMul:
            case tokens::kRem:
            case tokens::kAnd:
                return CheckUnaryAddressExpr(unary_expr);
            default:
                throw "internal error: unexpected unary op";
        }
    } else if (ast::BinaryExpr *binary_expr = dynamic_cast<ast::BinaryExpr *>(expr)) {
        switch (binary_expr->op()) {
            case tokens::kAdd:
            case tokens::kSub:
            case tokens::kMul:
            case tokens::kQuo:
            case tokens::kRem:
            case tokens::kAnd:
            case tokens::kOr:
            case tokens::kXor:
            case tokens::kAndNot:
                return CheckBinaryArithmeticOrBitExpr(binary_expr);
            case tokens::kShl:
            case tokens::kShr:
                return CheckBinaryShiftExpr(binary_expr);
            case tokens::kLAnd:
            case tokens::kLOr:
                return CheckBinaryLogicExpr(binary_expr);
            default:
                throw "internal error: unexpected binary op";
        }
    } else if (ast::CompareExpr *compare_expr = dynamic_cast<ast::CompareExpr *>(expr)) {
        return CheckCompareExpr(compare_expr);
    } else if (ast::ParenExpr *paren_expr = dynamic_cast<ast::ParenExpr *>(expr)) {
        return CheckParenExpr(paren_expr);
    } else if (ast::SelectionExpr *selection_expr = dynamic_cast<ast::SelectionExpr *>(expr)) {
        return CheckSelectionExpr(selection_expr);
    } else if (ast::TypeAssertExpr *type_assert_expr = dynamic_cast<ast::TypeAssertExpr *>(expr)) {
        return CheckTypeAssertExpr(type_assert_expr);
    } else if (ast::IndexExpr *index_expr = dynamic_cast<ast::IndexExpr *>(expr)) {
        return CheckIndexExpr(index_expr);
    } else if (ast::CallExpr *call_expr = dynamic_cast<ast::CallExpr *>(expr)) {
        return CheckCallExpr(call_expr);
    } else if (ast::FuncLit *func_lit = dynamic_cast<ast::FuncLit *>(expr)) {
        return CheckFuncLit(func_lit);
    } else if (ast::CompositeLit *composite_lit = dynamic_cast<ast::CompositeLit *>(expr)) {
        return CheckCompositeLit(composite_lit);
    } else if (ast::BasicLit *basic_lit = dynamic_cast<ast::BasicLit *>(expr)) {
        return CheckBasicLit(basic_lit);
    } else if (ast::Ident *ident = dynamic_cast<ast::Ident *>(expr)) {
        return CheckIdent(ident);
    } else {
        throw "unexpected AST expr";
    }
}

bool ExprHandler::CheckUnaryArithmeticOrBitExpr(ast::UnaryExpr *unary_expr) {
    if (!CheckExpr(unary_expr->x())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(unary_expr->x());
    types::Basic *underlying_type = dynamic_cast<types::Basic *>(x_type->Underlying());
    if (underlying_type == nullptr ||
        !(underlying_type->info() & types::Basic::Info::kIsInteger)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        unary_expr->start(),
                                        "invalid operation: expected integer type"));
        return false;
    }
    info_builder_.SetExprType(unary_expr, x_type);
    info_builder_.SetExprKind(unary_expr, types::ExprKind::kValue);
    return true;
}

bool ExprHandler::CheckUnaryLogicExpr(ast::UnaryExpr *unary_expr) {
    if (!CheckExpr(unary_expr->x())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(unary_expr->x());
    types::Basic *underlying_type = dynamic_cast<types::Basic *>(x_type->Underlying());
    if (underlying_type == nullptr ||
        !(underlying_type->info() & types::Basic::Info::kIsBoolean)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        unary_expr->start(),
                                        "invalid operation: expected boolean type"));
        return false;
    }
    info_builder_.SetExprType(unary_expr, x_type);
    info_builder_.SetExprKind(unary_expr, types::ExprKind::kValue);
    return true;
}

bool ExprHandler::CheckUnaryAddressExpr(ast::UnaryExpr *unary_expr) {
    if (!CheckExpr(unary_expr->x())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(unary_expr->x());
    if (unary_expr->op() == tokens::kAnd) {
        if (info_->ExprKindOf(unary_expr->x()).value() != types::ExprKind::kVariable) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            unary_expr->start(),
                                            "invalid operation: expected addressable value"));
            return false;
        }
        types::Pointer *pointer_type = info_builder_.CreatePointer(types::Pointer::Kind::kStrong,
                                                                   x_type);
        info_builder_.SetExprType(unary_expr, pointer_type);
        info_builder_.SetExprKind(unary_expr, types::ExprKind::kValue);
        return true;
        
    } else if (unary_expr->op() == tokens::kMul ||
               unary_expr->op() == tokens::kRem) {
        types::Pointer *x_pointer_type = dynamic_cast<types::Pointer *>(x_type);
        if (x_pointer_type == nullptr) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            unary_expr->start(),
                                            "invalid operation: expected pointer"));
            return false;
        }
        if (x_pointer_type->kind() == types::Pointer::Kind::kStrong &&
            unary_expr->op() == tokens::kRem) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            unary_expr->start(),
                                            "invalid operation: can not weakly dereference strong "
                                            "pointer"));
            return false;
        } else if (x_pointer_type->kind() == types::Pointer::Kind::kWeak &&
                   unary_expr->op() == tokens::kMul) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            unary_expr->start(),
                                            "invalid operation: can not strongly dereference weak "
                                            "pointer"));
            return false;
        }
        info_builder_.SetExprType(unary_expr, x_pointer_type->element_type());
        info_builder_.SetExprKind(unary_expr, types::ExprKind::kVariable);
        return true;
        
    } else {
        throw "internal error: unexpected unary operand";
    }
}

bool ExprHandler::CheckBinaryArithmeticOrBitExpr(ast::BinaryExpr *binary_expr) {
    if (!CheckExpr(binary_expr->x()) ||
        !CheckExpr(binary_expr->y())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(binary_expr->x());
    types::Type *y_type = info_->TypeOf(binary_expr->y());
    types::Basic *x_underlying_type = dynamic_cast<types::Basic *>(x_type->Underlying());
    types::Basic *y_underlying_type = dynamic_cast<types::Basic *>(y_type->Underlying());
    if (x_underlying_type == nullptr || y_underlying_type == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        binary_expr->start(),
                                        "invalid operation: expected basic type"));
        return false;
    } else if (!(x_underlying_type->info() & types::Basic::Info::kIsUntyped) &&
               !(y_underlying_type->info() & types::Basic::Info::kIsUntyped) &&
               x_underlying_type != y_underlying_type) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        binary_expr->start(),
                                        "invalid operation: mismatched types"));
        return false;
    }
    
    if (binary_expr->op() == tokens::kAdd) {
        if (!(x_underlying_type->info() & types::Basic::Info::kIsString) &&
            !(y_underlying_type->info() & types::Basic::Info::kIsNumeric)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            binary_expr->start(),
                                            "invalid operation: expected string or numeric type"));
            return false;
        }
    } else {
        if (!(x_underlying_type->info() & types::Basic::Info::kIsNumeric)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            binary_expr->start(),
                                            "invalid operation: expected numeric type"));
            return false;
        }
    }
    types::Type *expr_type;
    if (!(x_underlying_type->info() & types::Basic::Info::kIsUntyped)) {
        expr_type = x_type;
    } else {
        expr_type = y_type;
    }
    info_builder_.SetExprType(binary_expr, expr_type);
    info_builder_.SetExprKind(binary_expr, types::ExprKind::kValue);
    return true;
}

bool ExprHandler::CheckBinaryShiftExpr(ast::BinaryExpr *binary_expr) {
    if (!CheckExpr(binary_expr->x()) ||
        !CheckExpr(binary_expr->y())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(binary_expr->x());
    types::Type *y_type = info_->TypeOf(binary_expr->y());
    types::Basic *x_underlying_type = dynamic_cast<types::Basic *>(x_type->Underlying());
    types::Basic *y_underlying_type = dynamic_cast<types::Basic *>(y_type->Underlying());
    if (x_underlying_type == nullptr || y_underlying_type == nullptr ||
        !(x_underlying_type->info() & types::Basic::kIsNumeric) ||
        !(y_underlying_type->info() & types::Basic::kIsNumeric)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        binary_expr->start(),
                                        "invalid operation: expected numeric types"));
        return false;
    }

    types::Type *expr_type = x_type;
    if (x_underlying_type->info() & types::Basic::Info::kIsUntyped) {
        expr_type = info_->basic_type(types::Basic::Kind::kInt);
    }
    info_builder_.SetExprType(binary_expr, expr_type);
    info_builder_.SetExprKind(binary_expr, types::ExprKind::kValue);
    return true;
}

bool ExprHandler::CheckBinaryLogicExpr(ast::BinaryExpr *binary_expr) {
    if (!CheckExpr(binary_expr->x()) ||
        !CheckExpr(binary_expr->y())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(binary_expr->x());
    types::Type *y_type = info_->TypeOf(binary_expr->y());
    types::Basic *x_underlying_type = dynamic_cast<types::Basic *>(x_type->Underlying());
    types::Basic *y_underlying_type = dynamic_cast<types::Basic *>(y_type->Underlying());
    if (x_underlying_type == nullptr || y_underlying_type == nullptr ||
        !(x_underlying_type->info() & types::Basic::Info::kIsBoolean) ||
        !(y_underlying_type->info() & types::Basic::Info::kIsBoolean)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        binary_expr->start(),
                                        "invalid operation: expected boolean type"));
        return false;
    } else if (!(x_underlying_type->info() & types::Basic::Info::kIsUntyped) &&
               !(y_underlying_type->info() & types::Basic::Info::kIsUntyped) &&
               x_underlying_type != y_underlying_type) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        binary_expr->start(),
                                        "invalid operation: mismatched types"));
        return false;
    }
    types::Type *expr_type;
    if (!(x_underlying_type->info() & types::Basic::Info::kIsUntyped)) {
        expr_type = x_type;
    } else {
        expr_type = y_type;
    }
    info_builder_.SetExprType(binary_expr, expr_type);
    info_builder_.SetExprKind(binary_expr, types::ExprKind::kValue);
    return true;
}

bool ExprHandler::CheckCompareExpr(ast::CompareExpr *compare_expr) {
    for (ast::Expr *operand : compare_expr->operands()) {
        if (!CheckExpr(operand)) {
            return false;
        }
        types::Type *type = info_->TypeOf(operand);
        types::Basic *underlying_type = dynamic_cast<types::Basic *>(type->Underlying());
        if (underlying_type == nullptr) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            operand->start(),
                                            "invalid operation: expected basic type"));
            return false;
        }
    }
    for (int i = 0; i < compare_expr->compare_ops().size(); i++) {
        tokens::Token op = compare_expr->compare_ops().at(i);
        ast::Expr *x = compare_expr->operands().at(i);
        ast::Expr *y = compare_expr->operands().at(i + 1);
        types::Type *x_type = info_->TypeOf(x);
        types::Type *y_type = info_->TypeOf(y);
        types::Basic *x_underlying_type = static_cast<types::Basic *>(x_type->Underlying());
        types::Basic *y_underlying_type = static_cast<types::Basic *>(y_type->Underlying());
        if (!(x_underlying_type->info() & types::Basic::Info::kIsUntyped) &&
            !(y_underlying_type->info() & types::Basic::Info::kIsUntyped) &&
            x_underlying_type != y_underlying_type) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            compare_expr->compare_op_starts().at(i),
                                            "invalid operation: mismatched types"));
            return false;
        }
        
        if (op == tokens::kLss || op == tokens::kGtr || op == tokens::kGeq || op == tokens::kLeq) {
            if (!(x_underlying_type->info() & types::Basic::Info::kIsOrdered)) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                compare_expr->compare_op_starts().at(i),
                                                "invalid operation: expected orderable type"));
                return false;
            }
        }
    }
    info_builder_.SetExprType(compare_expr, info_->basic_type(types::Basic::Kind::kBool));
    info_builder_.SetExprKind(compare_expr, types::ExprKind::kValue);
    return true;
}

bool ExprHandler::CheckParenExpr(ast::ParenExpr *paren_expr) {
    if (!CheckExpr(paren_expr->x())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(paren_expr->x());
    types::ExprKind x_expr_kind = info_->ExprKindOf(paren_expr->x()).value();
    info_builder_.SetExprType(paren_expr, x_type);
    info_builder_.SetExprKind(paren_expr, x_expr_kind);
    return true;
}

bool ExprHandler::CheckSelectionExpr(ast::SelectionExpr *selection_expr) {
    if (ast::Ident *accessed_ident = dynamic_cast<ast::Ident *>(selection_expr->accessed())) {
        types::Object *accessed_obj = info_->UseOf(accessed_ident);
        types::PackageName *pkg_name = dynamic_cast<types::PackageName *>(accessed_obj);
        if (pkg_name != nullptr) {
            if (!CheckIdent(selection_expr->selection())) {
                return false;
            }
            types::Type *type = info_->TypeOf(selection_expr->selection());
            types::ExprKind expr_kind = info_->ExprKindOf(selection_expr->selection()).value();
            info_builder_.SetExprType(selection_expr, type);
            info_builder_.SetExprKind(selection_expr, expr_kind);
            return true;
        }
    }
    
    if (!CheckExpr(selection_expr->accessed())) {
        return false;
    }
    types::Type *accessed_type = info_->TypeOf(selection_expr->accessed());
    types::ExprKind accessed_kind = info_->ExprKindOf(selection_expr->accessed()).value();
    if (types::Pointer *pointer_type = dynamic_cast<types::Pointer *>(accessed_type)) {
        accessed_type = pointer_type->element_type();
        if (nullptr != dynamic_cast<types::Interface *>(accessed_type->Underlying()) ||
            nullptr != dynamic_cast<types::TypeParameter *>(accessed_type)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            selection_expr->selection()->start(),
                                            "invalid operation: selection from pointer to "
                                            "interface or type parameter not allowed"));
            return false;
        }
    }
    
    std::string selection_name = selection_expr->selection()->name();
    types::NamedType *named_type = dynamic_cast<types::NamedType *>(accessed_type);
    types::InfoBuilder::TypeParamsToArgsMap type_params_to_args;
    if (auto type_instance = dynamic_cast<types::TypeInstance *>(accessed_type)) {
        accessed_type = type_instance->instantiated_type();
        named_type = type_instance->instantiated_type();
        for (int i = 0; i < type_instance->type_args().size(); i++) {
            types::TypeParameter *type_param = named_type->type_parameters().at(i);
            types::Type *type_arg = type_instance->type_args().at(i);
            type_params_to_args.insert({type_param, type_arg});
        }
        
    } else if (auto type_parameter = dynamic_cast<types::TypeParameter *>(accessed_type)) {
        accessed_type = type_parameter->interface();
        named_type = dynamic_cast<types::NamedType *>(accessed_type);
    }
    
    while (named_type != nullptr) {
        if (named_type->methods().contains(selection_name)) {
            types::Func *method = named_type->methods().at(selection_name);
            types::Signature *signature = static_cast<types::Signature *>(method->type());
            types::Type *receiver_type = nullptr;
            if (signature->expr_receiver() != nullptr) {
                receiver_type = signature->expr_receiver()->type();
                if (types::Pointer *pointer = dynamic_cast<types::Pointer *>(receiver_type)) {
                    receiver_type = pointer->element_type();
                }
            } else if (signature->type_receiver() != nullptr) {
                receiver_type = signature->type_receiver();
            }
            
            if (auto type_instance = dynamic_cast<types::TypeInstance *>(receiver_type)) {
                types::InfoBuilder::TypeParamsToArgsMap method_type_params_to_args;
                method_type_params_to_args.reserve(type_params_to_args.size());
                for (int i = 0; i < type_instance->type_args().size(); i++) {
                    types::TypeParameter *original_type_param = named_type->type_parameters().at(i);
                    types::TypeParameter *method_type_param =
                        static_cast<types::TypeParameter *>(type_instance->type_args().at(i));
                    types::Type *type_arg = type_params_to_args.at(original_type_param);
                    method_type_params_to_args[method_type_param] = type_arg;
                }
                type_params_to_args = method_type_params_to_args;
            }
            
            types::Selection *selection = nullptr;
            switch (accessed_kind) {
                case types::ExprKind::kConstant:
                case types::ExprKind::kVariable:
                case types::ExprKind::kValue:
                case types::ExprKind::kValueOk:{
                    signature =
                    info_builder_.InstantiateMethodSignature(signature,
                                                             type_params_to_args,
                                                             /* receiver_to_arg= */ false);
                    selection = info_builder_.CreateSelection(types::Selection::Kind::kMethodVal,
                                                              named_type,
                                                              signature,
                                                              method);
                    break;
                }
                case types::ExprKind::kType:{
                    bool receiver_to_arg = (signature->expr_receiver() != nullptr);
                    signature = info_builder_.InstantiateMethodSignature(signature,
                                                                         type_params_to_args,
                                                                         receiver_to_arg);
                    selection = info_builder_.CreateSelection(types::Selection::Kind::kMethodExpr,
                                                              named_type,
                                                              signature,
                                                              method);
                    break;
                }
                default:
                    throw "internal error: unexpected expr kind of named type with method";
            }
            info_builder_.SetSelection(selection_expr, selection);
            info_builder_.SetExprType(selection_expr, signature);
            info_builder_.SetExprKind(selection_expr, types::ExprKind::kValue);
            return true;
        }
        accessed_type = named_type->type();
        named_type = dynamic_cast<types::NamedType *>(accessed_type);
    }
    
    switch (accessed_kind) {
        case types::ExprKind::kVariable:
        case types::ExprKind::kValue:
        case types::ExprKind::kValueOk:
            if (auto struct_type = dynamic_cast<types::Struct *>(accessed_type)) {
                for (types::Variable *field : struct_type->fields()) {
                    if (field->name() != selection_name) {
                        continue;
                    }
                    types::Type *field_type = field->type();
                    field_type = info_builder_.InstantiateType(field_type,
                                                               type_params_to_args);
                    types::Selection *selection =
                        info_builder_.CreateSelection(types::Selection::Kind::kFieldVal,
                                                      struct_type,
                                                      field_type,
                                                      field);
                    info_builder_.SetSelection(selection_expr, selection);
                    info_builder_.SetExprType(selection_expr, field_type);
                    info_builder_.SetExprKind(selection_expr, types::ExprKind::kVariable);
                    return true;
                }
                
            }
            // fallthrough
        case types::ExprKind::kType:
            if (auto interface_type = dynamic_cast<types::Interface *>(accessed_type)) {
                for (types::Func *method : interface_type->methods()) {
                    if (method->name() != selection_name) {
                        continue;
                    }
                    types::Signature *signature = static_cast<types::Signature *>(method->type());
                    if (signature->type_receiver()) {
                        types::TypeParameter *type_parameter =
                            static_cast<types::TypeParameter *>(signature->type_receiver());
                        type_params_to_args[type_parameter] = interface_type;
                    }
                    signature =
                        info_builder_.InstantiateMethodSignature(signature,
                                                                 type_params_to_args,
                                                                 /* receiver_to_arg= */ false);
                    types::Selection *selection =
                        info_builder_.CreateSelection(types::Selection::Kind::kMethodVal,
                                                      interface_type,
                                                      method->type(),
                                                      method);
                    info_builder_.SetSelection(selection_expr, selection);
                    info_builder_.SetExprType(selection_expr, signature);
                    info_builder_.SetExprKind(selection_expr, types::ExprKind::kValue);
                    return true;
                }
            }
        default:
            break;
    }
    
    issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                    issues::Severity::Error,
                                    selection_expr->selection()->start(),
                                    "could not resolve selection"));
    return false;
}

bool ExprHandler::CheckTypeAssertExpr(ast::TypeAssertExpr *type_assert_expr) {
    if (type_assert_expr->type() == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_assert_expr->start(),
                                        "invalid operation: blank type assertion outside type "
                                        "switch"));
        return false;
    }
    if (!CheckExpr(type_assert_expr->x()) ||
        !CheckExpr(type_assert_expr->type())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(type_assert_expr->x());
    types::Type *asserted_type = info_->TypeOf(type_assert_expr->type());
    types::Interface *x_interface = dynamic_cast<types::Interface *>(x_type);
    if (x_interface == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_assert_expr->start(),
                                        "invalid operation: expected interface value"));
        return false;
    }
    if (!types::IsAssertableTo(x_interface, asserted_type)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_assert_expr->start(),
                                        "invalid operation: interface is not assertable"));
        return false;
    }
    
    info_builder_.SetExprType(type_assert_expr, asserted_type);
    info_builder_.SetExprKind(type_assert_expr, types::ExprKind::kValueOk);
    return true;
}

bool ExprHandler::CheckIndexExpr(ast::IndexExpr *index_expr) {
    if (!CheckExpr(index_expr->accessed()) ||
        !CheckExpr(index_expr->index())) {
        return false;
    }
    
    types::Type *accessed_type = info_->TypeOf(index_expr->accessed());
    types::Type *index_type = info_->TypeOf(index_expr->index());
    types::Basic *index_basic_type = dynamic_cast<types::Basic *>(index_type->Underlying());
    if (index_basic_type == nullptr ||
        !(index_basic_type->kind() & (types::Basic::Kind::kInt |
                                      types::Basic::Kind::kUntypedInt))) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        index_expr->start(),
                                        "invalid operation: expected integer value"));
        return false;
    }
    
    auto add_expected_accessed_value_issue = [&]() {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        index_expr->start(),
                                        "invalid operation: expected array, pointer to array, "
                                        "slice, or string"));
    };
    if (auto pointer_type = dynamic_cast<types::Pointer *>(accessed_type->Underlying())) {
        auto element_type = dynamic_cast<types::Array *>(pointer_type->element_type());
        if (element_type == nullptr) {
            add_expected_accessed_value_issue();
            return false;
        }
        accessed_type = element_type;
    }
    if (auto array_type = dynamic_cast<types::Array *>(accessed_type->Underlying())) {
        types::Type *element_type = array_type->element_type();
        
        info_builder_.SetExprType(index_expr, element_type);
        info_builder_.SetExprKind(index_expr, types::ExprKind::kVariable);
        return true;
        
    } else if (auto slice_type = dynamic_cast<types::Slice *>(accessed_type->Underlying())) {
        types::Type *element_type = slice_type->element_type();
        
        info_builder_.SetExprType(index_expr, element_type);
        info_builder_.SetExprKind(index_expr, types::ExprKind::kVariable);
        return true;
        
    } else if (auto string_type = dynamic_cast<types::Basic *>(accessed_type->Underlying())) {
        if (!(string_type->info() & types::Basic::Info::kIsString)) {
            add_expected_accessed_value_issue();
            return false;
        }
        
        info_builder_.SetExprType(index_expr, info_->basic_type(types::Basic::Kind::kByte));
        info_builder_.SetExprKind(index_expr, types::ExprKind::kValue);
        return true;
        
    } else {
        add_expected_accessed_value_issue();
        return false;
    }
}

bool ExprHandler::CheckCallExpr(ast::CallExpr *call_expr) {
    ast::Expr *func_expr = call_expr->func();
    
    if (!CheckExpr(func_expr) ||
        (!call_expr->type_args().empty() &&
         !TypeHandler::ProcessTypeArgs(call_expr->type_args(), info_builder_, issues_)) ||
        !CheckExprs(call_expr->args())) {
        return false;
    }
    
    if (info_->ExprKindOf(func_expr) == types::ExprKind::kType) {
        return CheckCallExprWithTypeConversion(call_expr);
    } else if (info_->ExprKindOf(func_expr) == types::ExprKind::kBuiltin) {
        return CheckCallExprWithBuiltin(call_expr);
    } else if (info_->ExprKindOf(func_expr) == types::ExprKind::kValue ||
               info_->ExprKindOf(func_expr) == types::ExprKind::kVariable) {
        return CheckCallExprWithFuncCall(call_expr);
    } else {
        throw "internal error: unexpected func expr kind";
    }
}

bool ExprHandler::CheckCallExprWithTypeConversion(ast::CallExpr *call_expr) {
    if (!call_expr->type_args().empty()) {
        // TODO: handle this in future
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        call_expr->start(),
                                        "invalid operation: type conversion does not accept "
                                        "type arguments"));
        return false;
    }
    if (call_expr->args().size() != 1) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        call_expr->start(),
                                        "invalid operation: type conversion requires exactly "
                                        "one argument"));
        return false;
    }
    types::Type *conversion_start_type = info_->TypeOf(call_expr->args().at(0));
    types::Type *conversion_result_type = info_->TypeOf(call_expr->func());
    if (!types::IsConvertibleTo(conversion_start_type, conversion_result_type)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        call_expr->start(),
                                        "invalid operation: type conversion not possible"));
        return false;
    }
    
    info_builder_.SetExprType(call_expr, conversion_result_type);
    info_builder_.SetExprKind(call_expr, types::ExprKind::kValue);
    return true;
}

bool ExprHandler::CheckCallExprWithBuiltin(ast::CallExpr *call_expr) {
    ast::Ident *builtin_ident = static_cast<ast::Ident *>(ast::Unparen(call_expr->func()));
    types::Builtin *builtin = static_cast<types::Builtin *>(info_->UseOf(builtin_ident));
    
    switch (builtin->kind()) {
        case types::Builtin::Kind::kLen:{
            if (!call_expr->type_args().empty()) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                call_expr->start(),
                                                "len does not accept type arguments"));
                return false;
            }
            if (call_expr->args().size() != 1) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                call_expr->l_paren(),
                                                "len expected one argument"));
                return false;
            }
            ast::Expr *arg_expr = call_expr->args().at(0);
            types::Type *arg_type = info_->TypeOf(arg_expr)->Underlying();
            types::Basic *basic = dynamic_cast<types::Basic *>(arg_type);
            if (!(basic != nullptr && basic->kind() == types::Basic::kString) &&
                dynamic_cast<types::Array *>(arg_type) == nullptr &&
                dynamic_cast<types::Slice *>(arg_type) == nullptr) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                arg_expr->start(),
                                                "len expected array, slice, or string"));
                return false;
            }
            info_builder_.SetExprType(call_expr, info_->basic_type(types::Basic::kInt));
            info_builder_.SetExprKind(call_expr, types::ExprKind::kValue);
            return true;
        }
        case types::Builtin::Kind::kMake:{
            if (call_expr->type_args().size() != 1) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                call_expr->start(),
                                                "make expected one type argument"));
                return false;
            }
            if (call_expr->args().size() != 1) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                call_expr->l_paren(),
                                                "make expected one argument"));
                return false;
            }
            ast::Expr *slice_expr = call_expr->type_args().at(0);
            types::Slice *slice = dynamic_cast<types::Slice *>(info_->TypeOf(slice_expr));
            if (slice == nullptr) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                slice_expr->start(),
                                                "make expected slice type argument"));
                return false;
            }
            ast::Expr *length_expr = call_expr->args().at(0);
            types::Basic *length_type = dynamic_cast<types::Basic *>(info_->TypeOf(length_expr));
            if (length_type == nullptr || (length_type->kind() != types::Basic::kInt &&
                                           length_type->kind() != types::Basic::kUntypedInt)) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                length_expr->start(),
                                                "make expected length of type int"));
                return false;
            }
            info_builder_.SetExprType(call_expr, slice);
            info_builder_.SetExprKind(call_expr, types::ExprKind::kValue);
            return true;
        }
        case types::Builtin::Kind::kNew:{
            if (call_expr->type_args().size() != 1) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                call_expr->start(),
                                                "new expected one type argument"));
                return false;
            }
            if (!call_expr->args().empty()) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                call_expr->l_paren(),
                                                "new did not expect any arguments"));
                return false;
            }
            ast::Expr *element_type_expr = call_expr->type_args().at(0);
            types::Type *element_type = info_->TypeOf(element_type_expr);
            types::Pointer *pointer = info_builder_.CreatePointer(types::Pointer::Kind::kStrong,
                                                                  element_type);
            info_builder_.SetExprType(call_expr, pointer);
            info_builder_.SetExprKind(call_expr, types::ExprKind::kValue);
            return true;
        }
        default:
            throw "interal error: unexpected builtin kind";
    }
}

bool ExprHandler::CheckCallExprWithFuncCall(ast::CallExpr *call_expr) {
    types::Type *func_type = info_->TypeOf(call_expr->func())->Underlying();
    types::Signature *signature = dynamic_cast<types::Signature *>(func_type);
    if (signature == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        call_expr->start(),
                                        "expected type, function or function variable"));
        return false;
    }
    if (!signature->type_parameters().empty()) {
        signature = CheckFuncCallTypeArgs(signature, call_expr->type_args());
        if (signature == nullptr) {
            return false;
        }
    }
    CheckFuncCallArgs(signature, call_expr, call_expr->args());
    CheckFuncCallResultType(signature, call_expr);
    return true;
}

types::Signature * ExprHandler::CheckFuncCallTypeArgs(types::Signature *signature,
                                                      std::vector<ast::Expr *> type_arg_exprs) {
    size_t given_type_args = type_arg_exprs.size();
    size_t expected_type_args = signature->type_parameters().size();
    if (given_type_args != expected_type_args) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_arg_exprs.at(0)->start(),
                                        "expected " + std::to_string(expected_type_args) +
                                        " type arugments"));
        return nullptr;
    }
    std::unordered_map<types::TypeParameter *, types::Type *> type_params_to_args;
    for (int i = 0; i < expected_type_args; i++) {
        ast::Expr *type_arg_expr = type_arg_exprs.at(i);
        types::Type *type_arg = info_->TypeOf(type_arg_expr);
        types::TypeParameter *type_param = signature->type_parameters().at(i);
        
        if (!types::IsAssignableTo(type_arg, type_param)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            type_arg_expr->start(),
                                            "can not assign type argument to parameter"));
            return nullptr;
        }
        type_params_to_args.insert({type_param, type_arg});
    }
    types::Type *instantiated_signature =
        info_builder_.InstantiateFuncSignature(signature,
                                               type_params_to_args);
    return static_cast<types::Signature *>(instantiated_signature);
}

void ExprHandler::CheckFuncCallArgs(types::Signature *signature,
                                    ast::CallExpr *call_expr,
                                    std::vector<ast::Expr *> arg_exprs) {
    std::vector<types::Type *> arg_types;
    for (ast::Expr *arg_expr : arg_exprs) {
        arg_types.push_back(info_->TypeOf(arg_expr));
    }
    if (arg_types.size() == 1) {
        if (types::Tuple *tuple = dynamic_cast<types::Tuple *>(arg_types.at(0))) {
            arg_types.clear();
            for (types::Variable *var : tuple->variables()) {
                arg_types.push_back(var->type());
            }
        }
    }
    size_t expected_args = 0;
    if (signature->parameters() != nullptr) {
        expected_args = signature->parameters()->variables().size();
    }
    if (arg_types.size() != expected_args) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        call_expr->l_paren(),
                                        "expected " + std::to_string(expected_args) +
                                        " arugments"));
        return;
    }
    for (int i = 0; i < expected_args; i++) {
        types::Type *arg_type = arg_types.at(i);
        types::Type *param_type = signature->parameters()->variables().at(i)->type();
        
        if (!types::IsAssignableTo(arg_type, param_type)) {
            if (arg_exprs.size() == expected_args) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                arg_exprs.at(i)->start(),
                                                "can not assign argument to parameter"));
            } else {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                arg_exprs.at(0)->start(),
                                                "can not assign argument to parameter"));
                return;
            }
        }
    }
}

void ExprHandler::CheckFuncCallResultType(types::Signature *signature,
                                          ast::CallExpr *call_expr) {
    if (signature->results() == nullptr) {
        info_builder_.SetExprKind(call_expr, types::ExprKind::kNoValue);
        return;
    }
    
    if (signature->results()->variables().size() == 1) {
        info_builder_.SetExprType(call_expr, signature->results()->variables().at(0)->type());
        info_builder_.SetExprKind(call_expr, types::ExprKind::kValue);
        return;
    }
    
    info_builder_.SetExprType(call_expr, signature->results());
    info_builder_.SetExprKind(call_expr, types::ExprKind::kValue);
}

bool ExprHandler::CheckFuncLit(ast::FuncLit *func_lit) {
    ast::FuncType *func_type_expr = func_lit->type();
    ast::BlockStmt *func_body = func_lit->body();
    if (!TypeHandler::ProcessTypeExpr(func_type_expr, info_builder_, issues_)) {
        return false;
    }
    types::Signature *func_type = static_cast<types::Signature *>(info_->TypeOf(func_type_expr));
    
    StmtHandler::ProcessFuncBody(func_body, func_type->results(), info_builder_, issues_);
    
    info_builder_.SetExprType(func_lit, func_type);
    info_builder_.SetExprKind(func_lit, types::ExprKind::kValue);
    return true;
}

bool ExprHandler::CheckCompositeLit(ast::CompositeLit *composite_lit) {
    if (!TypeHandler::ProcessTypeExpr(composite_lit->type(), info_builder_, issues_)) {
        return false;
    }
    types::Type *type = info_->TypeOf(composite_lit->type());
    
    // TODO: check contents of composite lit
    
    info_builder_.SetExprType(composite_lit, type);
    info_builder_.SetExprKind(composite_lit, types::ExprKind::kValue);
    return true;
}

bool ExprHandler::CheckBasicLit(ast::BasicLit *basic_lit) {
    return ConstantHandler::ProcessConstantExpr(basic_lit, /* iota= */ 0, info_builder_, issues_);
}

bool ExprHandler::CheckIdent(ast::Ident *ident) {
    types::Object *object = info_->ObjectOf(ident);
    types::ExprKind expr_kind = types::ExprKind::kInvalid;
    if (dynamic_cast<types::TypeName *>(object) != nullptr) {
        expr_kind = types::ExprKind::kType;
        
    } else if (dynamic_cast<types::Constant *>(object) != nullptr) {
        expr_kind = types::ExprKind::kConstant;
        
    } else if (dynamic_cast<types::Variable *>(object) != nullptr) {
        expr_kind = types::ExprKind::kVariable;
        
    } else if (dynamic_cast<types::Func *>(object) != nullptr ||
               dynamic_cast<types::Nil *>(object) != nullptr) {
        expr_kind = types::ExprKind::kValue;
        
    } else if (dynamic_cast<types::Builtin *>(object) != nullptr) {
        expr_kind = types::ExprKind::kBuiltin;
        
    } else if (dynamic_cast<types::PackageName *>(object) != nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        ident->start(),
                                        "use of package name without selector"));
        return false;
    } else {
        throw "internal error: unexpected object type";
    }
    if (expr_kind != types::ExprKind::kBuiltin && object->type() == nullptr) {
        // TODO: uncomment exception when stmt handler is fully implemented
        throw "internal error: expect to know type at this point";
        return false;
    }
    
    if (expr_kind != types::ExprKind::kBuiltin) {
        info_builder_.SetExprType(ident, object->type());        
    }
    info_builder_.SetExprKind(ident, expr_kind);
    return true;
}

}
}

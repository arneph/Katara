//
//  expr_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "expr_handler.h"

#include <optional>

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
    switch (expr ->node_kind()) {
        case ast::NodeKind::kUnaryExpr:
            switch (static_cast<ast::UnaryExpr *>(expr)->op()) {
                case tokens::kAdd:
                case tokens::kSub:
                case tokens::kXor:
                    return CheckUnaryArithmeticOrBitExpr(static_cast<ast::UnaryExpr *>(expr));
                case tokens::kNot:
                    return CheckUnaryLogicExpr(static_cast<ast::UnaryExpr *>(expr));
                case tokens::kMul:
                case tokens::kRem:
                case tokens::kAnd:
                    return CheckUnaryAddressExpr(static_cast<ast::UnaryExpr *>(expr));
                default:
                    throw "internal error: unexpected unary op";
            }
        case ast::NodeKind::kBinaryExpr:
            switch (static_cast<ast::BinaryExpr *>(expr)->op()) {
                case tokens::kAdd:
                case tokens::kSub:
                case tokens::kMul:
                case tokens::kQuo:
                case tokens::kRem:
                case tokens::kAnd:
                case tokens::kOr:
                case tokens::kXor:
                case tokens::kAndNot:
                    return CheckBinaryArithmeticOrBitExpr(static_cast<ast::BinaryExpr *>(expr));
                case tokens::kShl:
                case tokens::kShr:
                    return CheckBinaryShiftExpr(static_cast<ast::BinaryExpr *>(expr));
                case tokens::kLAnd:
                case tokens::kLOr:
                    return CheckBinaryLogicExpr(static_cast<ast::BinaryExpr *>(expr));
                default:
                    throw "internal error: unexpected binary op";
            }
        case ast::NodeKind::kCompareExpr:
            return CheckCompareExpr(static_cast<ast::CompareExpr *>(expr));
        case ast::NodeKind::kParenExpr:
            return CheckParenExpr(static_cast<ast::ParenExpr *>(expr));
        case ast::NodeKind::kSelectionExpr:
            return CheckSelectionExpr(static_cast<ast::SelectionExpr *>(expr));
        case ast::NodeKind::kTypeAssertExpr:
            return CheckTypeAssertExpr(static_cast<ast::TypeAssertExpr *>(expr));
        case ast::NodeKind::kIndexExpr:
            return CheckIndexExpr(static_cast<ast::IndexExpr *>(expr));
        case ast::NodeKind::kCallExpr:
            return CheckCallExpr(static_cast<ast::CallExpr *>(expr));
        case ast::NodeKind::kFuncLit:
            return CheckFuncLit(static_cast<ast::FuncLit *>(expr));
        case ast::NodeKind::kCompositeLit:
            return CheckCompositeLit(static_cast<ast::CompositeLit *>(expr));
        case ast::NodeKind::kBasicLit:
            return CheckBasicLit(static_cast<ast::BasicLit *>(expr));
        case ast::NodeKind::kIdent:
            return CheckIdent(static_cast<ast::Ident *>(expr));
        default:
            throw "unexpected AST expr";
    }
}

bool ExprHandler::CheckUnaryArithmeticOrBitExpr(ast::UnaryExpr *unary_expr) {
    if (!CheckExpr(unary_expr->x())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(unary_expr->x());
    types::Type *x_underlying = types::UnderlyingOf(x_type);
    if (x_underlying == nullptr ||
        x_underlying->type_kind() != types::TypeKind::kBasic ||
        !(static_cast<types::Basic *>(x_underlying)->info() & types::Basic::Info::kIsInteger)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        unary_expr->start(),
                                        "invalid operation: expected integer type"));
        return false;
    }
    info_builder_.SetExprInfo(unary_expr, types::ExprInfo(types::ExprKind::kValue, x_type));
    return true;
}

bool ExprHandler::CheckUnaryLogicExpr(ast::UnaryExpr *unary_expr) {
    if (!CheckExpr(unary_expr->x())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(unary_expr->x());
    types::Type *x_underlying = types::UnderlyingOf(x_type);
    if (x_underlying == nullptr ||
        x_underlying->type_kind() != types::TypeKind::kBasic ||
        !(static_cast<types::Basic *>(x_underlying)->info() & types::Basic::Info::kIsBoolean)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        unary_expr->start(),
                                        "invalid operation: expected boolean type"));
        return false;
    }
    info_builder_.SetExprInfo(unary_expr, types::ExprInfo(types::ExprKind::kValue, x_type));
    return true;
}

bool ExprHandler::CheckUnaryAddressExpr(ast::UnaryExpr *unary_expr) {
    if (!CheckExpr(unary_expr->x())) {
        return false;
    }
    types::ExprInfo x_info = info_->ExprInfoOf(unary_expr->x()).value();
    if (unary_expr->op() == tokens::kAnd) {
        if (x_info.kind() != types::ExprKind::kVariable) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            unary_expr->start(),
                                            "invalid operation: expected addressable value"));
            return false;
        }
        types::Pointer *pointer_type = info_builder_.CreatePointer(types::Pointer::Kind::kStrong,
                                                                   x_info.type());
        info_builder_.SetExprInfo(unary_expr, types::ExprInfo(types::ExprKind::kValue,
                                                              pointer_type));
        return true;
        
    } else if (unary_expr->op() == tokens::kMul ||
               unary_expr->op() == tokens::kRem) {
        if (x_info.type()->type_kind() != types::TypeKind::kPointer) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            unary_expr->start(),
                                            "invalid operation: expected pointer"));
            return false;
        }
        types::Pointer *pointer = static_cast<types::Pointer *>(x_info.type());
        if (pointer->kind() == types::Pointer::Kind::kStrong &&
            unary_expr->op() == tokens::kRem) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            unary_expr->start(),
                                            "invalid operation: can not weakly dereference strong "
                                            "pointer"));
            return false;
        } else if (pointer->kind() == types::Pointer::Kind::kWeak &&
                   unary_expr->op() == tokens::kMul) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            unary_expr->start(),
                                            "invalid operation: can not strongly dereference weak "
                                            "pointer"));
            return false;
        }
        info_builder_.SetExprInfo(unary_expr, types::ExprInfo(types::ExprKind::kVariable,
                                                              pointer->element_type()));
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
    types::Type *x_underlying = types::UnderlyingOf(x_type);
    types::Type *y_underlying = types::UnderlyingOf(y_type);
    if (x_underlying == nullptr || x_underlying->type_kind() != types::TypeKind::kBasic ||
        y_underlying == nullptr || y_underlying->type_kind() != types::TypeKind::kBasic) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        binary_expr->start(),
                                        "invalid operation: expected basic type"));
        return false;
    }
    types::Basic *x_basic = static_cast<types::Basic *>(x_underlying);
    types::Basic *y_basic = static_cast<types::Basic *>(y_underlying);
    if (!(x_basic->info() & types::Basic::Info::kIsUntyped) &&
        !(y_basic->info() & types::Basic::Info::kIsUntyped) &&
        x_basic != y_basic) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        binary_expr->start(),
                                        "invalid operation: mismatched types"));
        return false;
    }
    
    if (binary_expr->op() == tokens::kAdd) {
        if (!(x_basic->info() & types::Basic::Info::kIsString) &&
            !(y_basic->info() & types::Basic::Info::kIsNumeric)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            binary_expr->start(),
                                            "invalid operation: expected string or numeric type"));
            return false;
        }
    } else {
        if (!(x_basic->info() & types::Basic::Info::kIsNumeric)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            binary_expr->start(),
                                            "invalid operation: expected numeric type"));
            return false;
        }
    }
    types::Type *expr_type;
    if (!(x_basic->info() & types::Basic::Info::kIsUntyped)) {
        expr_type = x_type;
    } else {
        expr_type = y_type;
    }
    info_builder_.SetExprInfo(binary_expr, types::ExprInfo(types::ExprKind::kValue, expr_type));
    return true;
}

bool ExprHandler::CheckBinaryShiftExpr(ast::BinaryExpr *binary_expr) {
    if (!CheckExpr(binary_expr->x()) ||
        !CheckExpr(binary_expr->y())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(binary_expr->x());
    types::Type *y_type = info_->TypeOf(binary_expr->y());
    types::Type *x_underlying = types::UnderlyingOf(x_type);
    types::Type *y_underlying = types::UnderlyingOf(y_type);
    if (x_underlying == nullptr || x_underlying->type_kind() != types::TypeKind::kBasic ||
        y_underlying == nullptr || y_underlying->type_kind() != types::TypeKind::kBasic) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        binary_expr->start(),
                                        "invalid operation: expected basic type"));
        return false;
    }
    types::Basic *x_basic = static_cast<types::Basic *>(x_underlying);
    types::Basic *y_basic = static_cast<types::Basic *>(y_underlying);
    if (!(x_basic->info() & types::Basic::kIsNumeric) ||
        !(y_basic->info() & types::Basic::kIsNumeric)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        binary_expr->start(),
                                        "invalid operation: expected numeric types"));
        return false;
    }

    types::Type *expr_type = x_type;
    if (x_basic->info() & types::Basic::Info::kIsUntyped) {
        expr_type = info_->basic_type(types::Basic::Kind::kInt);
    }
    info_builder_.SetExprInfo(binary_expr, types::ExprInfo(types::ExprKind::kValue, expr_type));
    return true;
}

bool ExprHandler::CheckBinaryLogicExpr(ast::BinaryExpr *binary_expr) {
    if (!CheckExpr(binary_expr->x()) ||
        !CheckExpr(binary_expr->y())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(binary_expr->x());
    types::Type *y_type = info_->TypeOf(binary_expr->y());
    types::Type *x_underlying = types::UnderlyingOf(x_type);
    types::Type *y_underlying = types::UnderlyingOf(y_type);
    if (x_underlying == nullptr || x_underlying->type_kind() != types::TypeKind::kBasic ||
        y_underlying == nullptr || y_underlying->type_kind() != types::TypeKind::kBasic) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        binary_expr->start(),
                                        "invalid operation: expected basic type"));
        return false;
    }
    types::Basic *x_basic = static_cast<types::Basic *>(x_underlying);
    types::Basic *y_basic = static_cast<types::Basic *>(y_underlying);
    if (!(x_basic->info() & types::Basic::Info::kIsBoolean) ||
        !(y_basic->info() & types::Basic::Info::kIsBoolean)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        binary_expr->start(),
                                        "invalid operation: expected boolean type"));
        return false;
    } else if (!(x_basic->info() & types::Basic::Info::kIsUntyped) &&
               !(y_basic->info() & types::Basic::Info::kIsUntyped) &&
               x_basic != y_basic) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        binary_expr->start(),
                                        "invalid operation: mismatched types"));
        return false;
    }
    types::Type *expr_type;
    if (!(x_basic->info() & types::Basic::Info::kIsUntyped)) {
        expr_type = x_type;
    } else {
        expr_type = y_type;
    }
    info_builder_.SetExprInfo(binary_expr, types::ExprInfo(types::ExprKind::kValue, expr_type));
    return true;
}

bool ExprHandler::CheckCompareExpr(ast::CompareExpr *compare_expr) {
    for (ast::Expr *operand : compare_expr->operands()) {
        if (!CheckExpr(operand)) {
            return false;
        }
        types::Type *type = info_->TypeOf(operand);
        types::Type *underlying = types::UnderlyingOf(type);
        if (underlying->type_kind() != types::TypeKind::kBasic) {
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
        types::Basic *x_basic = static_cast<types::Basic *>(types::UnderlyingOf(x_type));
        types::Basic *y_basic = static_cast<types::Basic *>(types::UnderlyingOf(y_type));
        if (!(x_basic->info() & types::Basic::Info::kIsUntyped) &&
            !(y_basic->info() & types::Basic::Info::kIsUntyped) &&
            x_basic != y_basic) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            compare_expr->compare_op_starts().at(i),
                                            "invalid operation: mismatched types"));
            return false;
        }
        
        if (op == tokens::kLss || op == tokens::kGtr || op == tokens::kGeq || op == tokens::kLeq) {
            if (!(x_basic->info() & types::Basic::Info::kIsOrdered)) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                compare_expr->compare_op_starts().at(i),
                                                "invalid operation: expected orderable type"));
                return false;
            }
        }
    }
    info_builder_.SetExprInfo(compare_expr,
                              types::ExprInfo(types::ExprKind::kValue,
                                              info_->basic_type(types::Basic::Kind::kBool)));
    return true;
}

bool ExprHandler::CheckParenExpr(ast::ParenExpr *paren_expr) {
    if (!CheckExpr(paren_expr->x())) {
        return false;
    }
    types::ExprInfo x_info = info_->ExprInfoOf(paren_expr->x()).value();
    info_builder_.SetExprInfo(paren_expr, x_info);
    return true;
}

bool ExprHandler::CheckSelectionExpr(ast::SelectionExpr *selection_expr) {
    switch (CheckPackageSelectionExpr(selection_expr)) {
        case CheckSelectionExprResult::kNotApplicable: break;
        case CheckSelectionExprResult::kCheckFailed: return false;
        case CheckSelectionExprResult::kCheckSucceeded: return true;
        default: throw "internal error: unexpected CheckSelectionExprResult";
    }
    
    if (!CheckExpr(selection_expr->accessed())) {
        return false;
    }
    types::ExprInfo accessed_info = info_->ExprInfoOf(selection_expr->accessed()).value();
    switch (accessed_info.kind()) {
        case types::ExprKind::kInvalid:
            throw "internal error: unexpected expr kind";
        case types::ExprKind::kNoValue:
        case types::ExprKind::kBuiltin:
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            selection_expr->selection()->start(),
                                            "invalid operation: no posible selection"));
            return false;
        default:
            break;
    }
    types::Type *accessed_type = accessed_info.type();
    if (accessed_type->type_kind() == types::TypeKind::kPointer) {
        accessed_type = static_cast<types::Pointer *>(accessed_type)->element_type();
        types::Type *underlying = types::UnderlyingOf(accessed_type);
        if ((underlying != nullptr &&
             underlying->type_kind() ==  types::TypeKind::kInterface) ||
            accessed_type->type_kind() == types::TypeKind::kTypeParameter) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            selection_expr->selection()->start(),
                                            "invalid operation: selection from pointer to "
                                            "interface or type parameter not allowed"));
            return false;
        }
    }
    if (accessed_type->type_kind() == types::TypeKind::kTypeParameter) {
        accessed_type = static_cast<types::TypeParameter *>(accessed_type)->interface();
    }
    types::InfoBuilder::TypeParamsToArgsMap type_params_to_args;
    if (accessed_type->type_kind() == types::TypeKind::kTypeInstance) {
        types::TypeInstance *type_instance = static_cast<types::TypeInstance *>(accessed_type);
        types::NamedType *instantiated_type = type_instance->instantiated_type();
        accessed_type = instantiated_type;
        for (int i = 0; i < type_instance->type_args().size(); i++) {
            types::TypeParameter *type_param = instantiated_type->type_parameters().at(i);
            types::Type *type_arg = type_instance->type_args().at(i);
            type_params_to_args.insert({type_param, type_arg});
        }
    }
    
    if (accessed_type->type_kind() == types::TypeKind::kNamedType) {
        types::NamedType *named_type = static_cast<types::NamedType *>(accessed_type);
        switch (CheckNamedTypeMethodSelectionExpr(selection_expr,
                                                  named_type,
                                                  type_params_to_args)) {
            case CheckSelectionExprResult::kNotApplicable: break;
            case CheckSelectionExprResult::kCheckFailed: return false;
            case CheckSelectionExprResult::kCheckSucceeded: return true;
            default: throw "internal error: unexpected CheckSelectionExprResult";
        }
        accessed_type = named_type->underlying();
    }
    switch (accessed_info.kind()) {
        case types::ExprKind::kVariable:
        case types::ExprKind::kValue:
        case types::ExprKind::kValueOk:
            switch (CheckStructFieldSelectionExpr(selection_expr,
                                                  accessed_type,
                                                  type_params_to_args)) {
                case CheckSelectionExprResult::kNotApplicable: break;
                case CheckSelectionExprResult::kCheckFailed: return false;
                case CheckSelectionExprResult::kCheckSucceeded: return true;
                default: throw "internal error: unexpected CheckSelectionExprResult";
            }
            // fallthrough
        case types::ExprKind::kType:
            switch (CheckInterfaceMethodSelectionExpr(selection_expr,
                                                      accessed_type,
                                                      type_params_to_args)) {
                case CheckSelectionExprResult::kNotApplicable: break;
                case CheckSelectionExprResult::kCheckFailed: return false;
                case CheckSelectionExprResult::kCheckSucceeded: return true;
                default: throw "internal error: unexpected CheckSelectionExprResult";
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

ExprHandler::CheckSelectionExprResult
ExprHandler::CheckPackageSelectionExpr(ast::SelectionExpr *selection_expr) {
    if (selection_expr->accessed()->node_kind() != ast::NodeKind::kIdent) {
        return CheckSelectionExprResult::kNotApplicable;
    }
    ast::Ident *accessed_ident = static_cast<ast::Ident *>(selection_expr->accessed());
    types::Object *accessed_obj = info_->UseOf(accessed_ident);
    if (accessed_obj->object_kind() != types::ObjectKind::kPackageName) {
        return CheckSelectionExprResult::kNotApplicable;
    }
    if (!CheckIdent(selection_expr->selection())) {
        return CheckSelectionExprResult::kCheckFailed;
    }
    types::ExprInfo selection_info = info_->ExprInfoOf(selection_expr->selection()).value();
    info_builder_.SetExprInfo(selection_expr, selection_info);
    return CheckSelectionExprResult::kCheckSucceeded;
}

ExprHandler::CheckSelectionExprResult
ExprHandler::CheckNamedTypeMethodSelectionExpr(ast::SelectionExpr *selection_expr,
                                               types::NamedType *named_type,
                                               types::InfoBuilder::TypeParamsToArgsMap
                                                   type_params_to_args) {
    types::ExprInfo accessed_info = info_->ExprInfoOf(selection_expr->accessed()).value();
    std::string selection_name = selection_expr->selection()->name();
    if (!named_type->methods().contains(selection_name)) {
        return CheckSelectionExprResult::kNotApplicable;
    }
    types::Func *method = named_type->methods().at(selection_name);
    types::Signature *signature = static_cast<types::Signature *>(method->type());
    types::Type *receiver_type = nullptr;
    if (signature->has_expr_receiver()) {
        receiver_type = signature->expr_receiver()->type();
        if (receiver_type->type_kind() == types::TypeKind::kPointer) {
            receiver_type = static_cast<types::Pointer *>(receiver_type)->element_type();
        }
    } else if (signature->has_type_receiver()) {
        receiver_type = signature->type_receiver();
    }
    
    if (receiver_type->type_kind() == types::TypeKind::kTypeInstance) {
        types::TypeInstance *type_instance = static_cast<types::TypeInstance *>(receiver_type);
        types::InfoBuilder::TypeParamsToArgsMap method_type_params_to_args;
        method_type_params_to_args.reserve(type_params_to_args.size());
        for (int i = 0; i < type_instance->type_args().size(); i++) {
            types::TypeParameter *original_type_param = named_type->type_parameters().at(i);
            types::TypeParameter *method_type_param =
                static_cast<types::TypeParameter *>(type_instance->type_args().at(i));
            types::Type *type_arg = type_params_to_args.at(original_type_param);
            method_type_params_to_args.insert({method_type_param, type_arg});
        }
        type_params_to_args = method_type_params_to_args;
    }
    
    types::Selection::Kind selection_kind;
    switch (accessed_info.kind()) {
        case types::ExprKind::kConstant:
        case types::ExprKind::kVariable:
        case types::ExprKind::kValue:
        case types::ExprKind::kValueOk:{
            signature = info_builder_.InstantiateMethodSignature(signature,
                                                                 type_params_to_args,
                                                                 /* receiver_to_arg= */ false);
            selection_kind = types::Selection::Kind::kMethodVal;
            break;
        }
        case types::ExprKind::kType:{
            bool receiver_to_arg = (signature->expr_receiver() != nullptr);
            signature = info_builder_.InstantiateMethodSignature(signature,
                                                                 type_params_to_args,
                                                                 receiver_to_arg);
            selection_kind = types::Selection::Kind::kMethodExpr;
            break;
        }
        default:
            throw "internal error: unexpected expr kind of named type with method";
    }
    types::Selection selection(types::Selection::Kind::kMethodExpr,
                               named_type,
                               signature,
                               method);
    info_builder_.SetSelection(selection_expr, selection);
    info_builder_.SetExprInfo(selection_expr, types::ExprInfo(types::ExprKind::kValue, signature));
    info_builder_.SetUsedObject(selection_expr->selection(), method);
    return CheckSelectionExprResult::kCheckSucceeded;
}

ExprHandler::CheckSelectionExprResult
ExprHandler::CheckInterfaceMethodSelectionExpr(ast::SelectionExpr *selection_expr,
                                               types::Type* accessed_type,
                                               types::InfoBuilder::TypeParamsToArgsMap
                                                   type_params_to_args) {
    std::string selection_name = selection_expr->selection()->name();
    if (accessed_type->type_kind() != types::TypeKind::kInterface) {
        return CheckSelectionExprResult::kNotApplicable;
    }
    types::Interface *interface_type = static_cast<types::Interface *>(accessed_type);
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
        types::Selection selection(types::Selection::Kind::kMethodVal,
                                   interface_type,
                                   method->type(),
                                   method);
        info_builder_.SetSelection(selection_expr, selection);
        info_builder_.SetExprInfo(selection_expr, types::ExprInfo(types::ExprKind::kValue,
                                                                  signature));
        info_builder_.SetUsedObject(selection_expr->selection(), method);
        return CheckSelectionExprResult::kCheckSucceeded;
    }
    return CheckSelectionExprResult::kNotApplicable;
}

ExprHandler::CheckSelectionExprResult
ExprHandler::CheckStructFieldSelectionExpr(ast::SelectionExpr *selection_expr,
                                           types::Type* accessed_type,
                                           types::InfoBuilder::TypeParamsToArgsMap
                                               type_params_to_args) {
    std::string selection_name = selection_expr->selection()->name();
    if (accessed_type->type_kind() != types::TypeKind::kStruct) {
        return CheckSelectionExprResult::kNotApplicable;
    }
    types::Struct *struct_type = static_cast<types::Struct *>(accessed_type);
    for (types::Variable *field : struct_type->fields()) {
        if (field->name() != selection_name) {
            continue;
        }
        types::Type *field_type = field->type();
        field_type = info_builder_.InstantiateType(field_type,
                                                   type_params_to_args);
        types::Selection selection(types::Selection::Kind::kFieldVal,
                                   struct_type,
                                   field_type,
                                   field);
        info_builder_.SetSelection(selection_expr, selection);
        info_builder_.SetExprInfo(selection_expr, types::ExprInfo(types::ExprKind::kVariable,
                                                                  field_type));
        info_builder_.SetUsedObject(selection_expr->selection(), field);
        return CheckSelectionExprResult::kCheckSucceeded;
    }
    return CheckSelectionExprResult::kNotApplicable;
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
    if (x_type->type_kind() != types::TypeKind::kInterface) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_assert_expr->start(),
                                        "invalid operation: expected interface value"));
        return false;
    }
    types::Interface *x_interface = static_cast<types::Interface *>(x_type);
    if (!types::IsAssertableTo(x_interface, asserted_type)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_assert_expr->start(),
                                        "invalid operation: interface is not assertable"));
        return false;
    }
    
    info_builder_.SetExprInfo(type_assert_expr, types::ExprInfo(types::ExprKind::kValueOk,
                                                                asserted_type));
    return true;
}

bool ExprHandler::CheckIndexExpr(ast::IndexExpr *index_expr) {
    if (!CheckExpr(index_expr->accessed()) ||
        !CheckExpr(index_expr->index())) {
        return false;
    }
    
    types::Type *index_type = info_->TypeOf(index_expr->index());
    types::Type *index_underlying = types::UnderlyingOf(index_type);
    if (index_underlying == nullptr ||
        index_underlying->type_kind() != types::TypeKind::kBasic ||
        !(static_cast<types::Basic *>(index_underlying)->kind()
          & (types::Basic::Kind::kInt | types::Basic::Kind::kUntypedInt))) {
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
    types::Type *accessed_type = info_->TypeOf(index_expr->accessed());
    types::Type *accessed_underlying = types::UnderlyingOf(accessed_type);
    if (accessed_underlying == nullptr) {
        add_expected_accessed_value_issue();
        return false;
    }
    if (accessed_underlying->type_kind() == types::TypeKind::kPointer) {
        types::Pointer *pointer_type = static_cast<types::Pointer *>(accessed_underlying);
        if (pointer_type->element_type()->type_kind() != types::TypeKind::kArray) {
            add_expected_accessed_value_issue();
            return false;
        }
        accessed_underlying = static_cast<types::Array *>(pointer_type->element_type());
    }
    if (accessed_underlying->is_container()) {
        types::Container *container = static_cast<types::Container *>(accessed_underlying);
        types::Type *element_type = container->element_type();
        
        info_builder_.SetExprInfo(index_expr, types::ExprInfo(types::ExprKind::kVariable,
                                                              element_type));
        return true;
        
    } else if (accessed_underlying->type_kind() == types::TypeKind::kBasic) {
        types::Basic *accessed_basic = static_cast<types::Basic *>(accessed_underlying);
        if (!(accessed_basic->info() & types::Basic::Info::kIsString)) {
            add_expected_accessed_value_issue();
            return false;
        }
        
        info_builder_.SetExprInfo(index_expr,
                                  types::ExprInfo(types::ExprKind::kValue,
                                                  info_->basic_type(types::Basic::Kind::kByte)));
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
    types::ExprInfo func_expr_info = info_->ExprInfoOf(func_expr).value();
    switch (func_expr_info.kind()) {
        case types::ExprKind::kBuiltin:
            return CheckCallExprWithBuiltin(call_expr);
        case types::ExprKind::kType:
            return CheckCallExprWithTypeConversion(call_expr);
        case types::ExprKind::kVariable:
        case types::ExprKind::kValue:
            return CheckCallExprWithFuncCall(call_expr);
        default:
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            call_expr->start(),
                                            "invalid operation: function expression is not "
                                            "callable"));
            return false;
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
    
    info_builder_.SetExprInfo(call_expr, types::ExprInfo(types::ExprKind::kValue,
                                                         conversion_result_type));
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
            types::Type *arg_type = types::UnderlyingOf(info_->TypeOf(arg_expr));
            if (arg_type == nullptr ||
                ((arg_type->type_kind() != types::TypeKind::kBasic ||
                  static_cast<types::Basic *>(arg_type)->kind() != types::Basic::kString) &&
                 arg_type->type_kind() != types::TypeKind::kArray &&
                 arg_type->type_kind() != types::TypeKind::kSlice)) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                arg_expr->start(),
                                                "len expected array, slice, or string"));
                return false;
            }
            info_builder_.SetExprInfo(call_expr,
                                      types::ExprInfo(types::ExprKind::kValue,
                                                      info_->basic_type(types::Basic::kInt)));
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
            if (info_->TypeOf(slice_expr)->type_kind() == types::TypeKind::kSlice) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                slice_expr->start(),
                                                "make expected slice type argument"));
                return false;
            }
            types::Slice *slice = static_cast<types::Slice *>(info_->TypeOf(slice_expr));
            ast::Expr *length_expr = call_expr->args().at(0);
            if (info_->TypeOf(length_expr)->type_kind() != types::TypeKind::kBasic) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                length_expr->start(),
                                                "make expected length of type int"));
                return false;
            }
            types::Basic *length_type = static_cast<types::Basic *>(info_->TypeOf(length_expr));
            if (length_type->kind() != types::Basic::kInt &&
                length_type->kind() != types::Basic::kUntypedInt) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                length_expr->start(),
                                                "make expected length of type int"));
                return false;
            }
            info_builder_.SetExprInfo(call_expr, types::ExprInfo(types::ExprKind::kValue, slice));
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
            info_builder_.SetExprInfo(call_expr, types::ExprInfo(types::ExprKind::kValue,
                                                                 pointer));
            return true;
        }
        default:
            throw "interal error: unexpected builtin kind";
    }
}

bool ExprHandler::CheckCallExprWithFuncCall(ast::CallExpr *call_expr) {
    types::Type *func_type = types::UnderlyingOf(info_->TypeOf(call_expr->func()));
    if (func_type->type_kind() != types::TypeKind::kSignature) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        call_expr->start(),
                                        "expected type, function or function variable"));
        return false;
    }
    types::Signature *signature = static_cast<types::Signature *>(func_type);
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
    return info_builder_.InstantiateFuncSignature(signature,
                                                  type_params_to_args);
}

void ExprHandler::CheckFuncCallArgs(types::Signature *signature,
                                    ast::CallExpr *call_expr,
                                    std::vector<ast::Expr *> arg_exprs) {
    std::vector<types::Type *> arg_types;
    for (ast::Expr *arg_expr : arg_exprs) {
        arg_types.push_back(info_->TypeOf(arg_expr));
    }
    if (arg_types.size() == 1 &&
        arg_types.at(0)->type_kind() == types::TypeKind::kTuple) {
        types::Tuple *tuple = static_cast<types::Tuple *>(arg_types.at(0));
        arg_types.clear();
        for (types::Variable *var : tuple->variables()) {
            arg_types.push_back(var->type());
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
        info_builder_.SetExprInfo(call_expr, types::ExprInfo(types::ExprKind::kNoValue, nullptr));
        return;
    }
    if (signature->results()->variables().size() == 1) {
        info_builder_.SetExprInfo(call_expr,
                                  types::ExprInfo(types::ExprKind::kValue,
                                                  signature->results()->variables().at(0)->type()));
        return;
    }
    info_builder_.SetExprInfo(call_expr, types::ExprInfo(types::ExprKind::kValue,
                                                         signature->results()));
}

bool ExprHandler::CheckFuncLit(ast::FuncLit *func_lit) {
    ast::FuncType *func_type_expr = func_lit->type();
    ast::BlockStmt *func_body = func_lit->body();
    if (!TypeHandler::ProcessTypeExpr(func_type_expr, info_builder_, issues_)) {
        return false;
    }
    types::Func *func = static_cast<types::Func *>(info_->ImplicitOf(func_lit));
    types::Signature *func_type = static_cast<types::Signature *>(info_->TypeOf(func_type_expr));
    
    StmtHandler::ProcessFuncBody(func_body, func_type->results(), info_builder_, issues_);
    
    info_builder_.SetObjectType(func, func_type);
    info_builder_.SetExprInfo(func_lit, types::ExprInfo(types::ExprKind::kValue, func_type));
    return true;
}

bool ExprHandler::CheckCompositeLit(ast::CompositeLit *composite_lit) {
    if (!TypeHandler::ProcessTypeExpr(composite_lit->type(), info_builder_, issues_)) {
        return false;
    }
    types::Type *type = info_->TypeOf(composite_lit->type());
    
    // TODO: check contents of composite lit
    
    info_builder_.SetExprInfo(composite_lit, types::ExprInfo(types::ExprKind::kValue, type));
    return true;
}

bool ExprHandler::CheckBasicLit(ast::BasicLit *basic_lit) {
    return ConstantHandler::ProcessConstantExpr(basic_lit, /* iota= */ 0, info_builder_, issues_);
}

bool ExprHandler::CheckIdent(ast::Ident *ident) {
    types::Object *object = info_->ObjectOf(ident);
    types::ExprKind expr_kind;
    types::Type *type = nullptr;
    switch (object->object_kind()) {
        case types::ObjectKind::kTypeName:
            expr_kind = types::ExprKind::kType;
            type = static_cast<types::TypeName *>(object)->type();
            break;
        case types::ObjectKind::kConstant:
            expr_kind = types::ExprKind::kConstant;
            type = static_cast<types::Constant *>(object)->type();
            break;
        case types::ObjectKind::kVariable:
            expr_kind = types::ExprKind::kVariable;
            type = static_cast<types::Variable *>(object)->type();
            break;
        case types::ObjectKind::kFunc:
            expr_kind = types::ExprKind::kVariable;
            type = static_cast<types::Func *>(object)->type();
            break;
        case types::ObjectKind::kNil:
            expr_kind = types::ExprKind::kValue;
            type = info_->basic_type(types::Basic::kUntypedNil);
            break;
        case types::ObjectKind::kBuiltin:
            expr_kind = types::ExprKind::kBuiltin;
            type = nullptr;
            break;
        case types::ObjectKind::kPackageName:
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            ident->start(),
                                            "use of package name without selector"));
            return false;
        default:
            throw "internal error: unexpected object type";
    }
    if (expr_kind != types::ExprKind::kBuiltin && type == nullptr) {
        throw "internal error: expect to know type at this point";
    }
    
    info_builder_.SetExprInfo(ident, types::ExprInfo(expr_kind, type));
    return true;
}

}
}

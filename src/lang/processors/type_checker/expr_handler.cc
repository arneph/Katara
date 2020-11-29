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
                              types::TypeInfo *info,
                              std::vector<issues::Issue>& issues) {
    ExprHandler handler(info, issues);
    
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
        switch (unary_expr->op_) {
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
        switch (binary_expr->op_) {
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
            case tokens::kEql:
            case tokens::kLss:
            case tokens::kGtr:
            case tokens::kNeq:
            case tokens::kLeq:
            case tokens::kGeq:
                return CheckBinaryComparisonExpr(binary_expr);
            default:
                throw "internal error: unexpected binary op";
        }
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
    if (!CheckExpr(unary_expr->x_.get())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(unary_expr->x_.get());
    types::Basic *underlying_type = dynamic_cast<types::Basic *>(x_type->Underlying());
    if (underlying_type == nullptr ||
        !(underlying_type->info() & types::Basic::Info::kIsInteger)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        unary_expr->start(),
                                        "invalid operation: expected integer type"));
        return false;
    }
    info_->types_.insert({unary_expr, x_type});
    info_->expr_kinds_.insert({unary_expr, types::ExprKind::kValue});
    return true;
}

bool ExprHandler::CheckUnaryLogicExpr(ast::UnaryExpr *unary_expr) {
    if (!CheckExpr(unary_expr->x_.get())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(unary_expr->x_.get());
    types::Basic *underlying_type = dynamic_cast<types::Basic *>(x_type->Underlying());
    if (underlying_type == nullptr ||
        !(underlying_type->info() & types::Basic::Info::kIsBoolean)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        unary_expr->start(),
                                        "invalid operation: expected boolean type"));
        return false;
    }
    info_->types_.insert({unary_expr, x_type});
    info_->expr_kinds_.insert({unary_expr, types::ExprKind::kValue});
    return true;
}

bool ExprHandler::CheckUnaryAddressExpr(ast::UnaryExpr *unary_expr) {
    if (!CheckExpr(unary_expr->x_.get())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(unary_expr->x_.get());
    if (unary_expr->op_ == tokens::kAnd) {
        if (info_->ExprKindOf(unary_expr->x_.get()).value() != types::ExprKind::kVariable) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            unary_expr->start(),
                                            "invalid operation: expected addressable value"));
            return false;
        }
        std::unique_ptr<types::Pointer> pointer_type(new types::Pointer());
        pointer_type->kind_ = types::Pointer::Kind::kStrong;
        pointer_type->element_type_ = x_type;
        
        types::Pointer *pointer_type_ptr = pointer_type.get();
        info_->type_unique_ptrs_.push_back(std::move(pointer_type));
        info_->types_.insert({unary_expr, pointer_type_ptr});
        info_->expr_kinds_.insert({unary_expr, types::ExprKind::kValue});
        return true;
        
    } else if (unary_expr->op_ == tokens::kMul ||
               unary_expr->op_ == tokens::kRem) {
        types::Pointer *x_pointer_type = dynamic_cast<types::Pointer *>(x_type);
        if (x_pointer_type == nullptr) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            unary_expr->start(),
                                            "invalid operation: expected pointer"));
            return false;
        }
        if (x_pointer_type->kind() == types::Pointer::Kind::kStrong &&
            unary_expr->op_ == tokens::kRem) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            unary_expr->start(),
                                            "invalid operation: can not weakly dereference strong "
                                            "pointer"));
            return false;
        } else if (x_pointer_type->kind() == types::Pointer::Kind::kWeak &&
                   unary_expr->op_ == tokens::kMul) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            unary_expr->start(),
                                            "invalid operation: can not strongly dereference weak "
                                            "pointer"));
            return false;
        }
        info_->types_.insert({unary_expr, x_pointer_type->element_type()});
        info_->expr_kinds_.insert({unary_expr, types::ExprKind::kVariable});
        return true;
        
    } else {
        throw "internal error: unexpected unary operand";
    }
}

bool ExprHandler::CheckBinaryArithmeticOrBitExpr(ast::BinaryExpr *binary_expr) {
    if (!CheckExpr(binary_expr->x_.get()) ||
        !CheckExpr(binary_expr->y_.get())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(binary_expr->x_.get());
    types::Type *y_type = info_->TypeOf(binary_expr->y_.get());
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
    
    if (binary_expr->op_ == tokens::kAdd) {
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
    info_->types_.insert({binary_expr, expr_type});
    info_->expr_kinds_.insert({binary_expr, types::ExprKind::kValue});
    return true;
}

bool ExprHandler::CheckBinaryShiftExpr(ast::BinaryExpr *binary_expr) {
    if (!CheckExpr(binary_expr->x_.get()) ||
        !CheckExpr(binary_expr->y_.get())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(binary_expr->x_.get());
    types::Type *y_type = info_->TypeOf(binary_expr->y_.get());
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
        expr_type = info_->basic_types_.at(types::Basic::Kind::kInt);
    }
    info_->types_.insert({binary_expr, expr_type});
    info_->expr_kinds_.insert({binary_expr, types::ExprKind::kValue});
    return true;
}

bool ExprHandler::CheckBinaryLogicExpr(ast::BinaryExpr *binary_expr) {
    if (!CheckExpr(binary_expr->x_.get()) ||
        !CheckExpr(binary_expr->y_.get())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(binary_expr->x_.get());
    types::Type *y_type = info_->TypeOf(binary_expr->y_.get());
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
    info_->types_.insert({binary_expr, expr_type});
    info_->expr_kinds_.insert({binary_expr, types::ExprKind::kValue});
    return true;
}

bool ExprHandler::CheckBinaryComparisonExpr(ast::BinaryExpr *binary_expr) {
    if (!CheckExpr(binary_expr->x_.get()) ||
        !CheckExpr(binary_expr->y_.get())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(binary_expr->x_.get());
    types::Type *y_type = info_->TypeOf(binary_expr->y_.get());
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
    
    if (binary_expr->op_ == tokens::kLss ||
        binary_expr->op_ == tokens::kGtr ||
        binary_expr->op_ == tokens::kGeq ||
        binary_expr->op_ == tokens::kLeq) {
        if (!(x_underlying_type->info() & types::Basic::Info::kIsOrdered)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            binary_expr->start(),
                                            "invalid operation: expected orderable type"));
            return false;
        }
    }
    info_->types_.insert({binary_expr, info_->basic_types_.at(types::Basic::Kind::kBool)});
    info_->expr_kinds_.insert({binary_expr, types::ExprKind::kValue});
    return true;
}

bool ExprHandler::CheckParenExpr(ast::ParenExpr *paren_expr) {
    if (!CheckExpr(paren_expr->x_.get())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(paren_expr->x_.get());
    types::ExprKind x_expr_kind = info_->ExprKindOf(paren_expr->x_.get()).value();
    info_->types_.insert({paren_expr, x_type});
    info_->expr_kinds_.insert({paren_expr, x_expr_kind});
    return true;
}

bool ExprHandler::CheckSelectionExpr(ast::SelectionExpr *selection_expr) {
    if (auto accessed_ident = dynamic_cast<ast::Ident *>(selection_expr->accessed_.get())) {
        types::Object *accessed_obj = info_->UseOf(accessed_ident);
        types::PackageName *pkg_name = dynamic_cast<types::PackageName *>(accessed_obj);
        if (pkg_name != nullptr) {
            if (!CheckIdent(selection_expr->selection_.get())) {
                return false;
            }
            types::Type *type = info_->TypeOf(selection_expr->selection_.get());
            types::ExprKind expr_kind = info_->ExprKindOf(selection_expr->selection_.get()).value();
            info_->types_.insert({selection_expr, type});
            info_->expr_kinds_.insert({selection_expr, expr_kind});
            return true;
        }
    }
    
    if (!CheckExpr(selection_expr->accessed_.get())) {
        return false;
    }
    types::Type *accessed_type = info_->TypeOf(selection_expr->accessed_.get());
    if (auto pointer_type = dynamic_cast<types::Pointer *>(accessed_type)) {
        accessed_type = pointer_type->element_type();
        if (dynamic_cast<types::Interface *>(accessed_type)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            selection_expr->selection_->start(),
                                            "invalid operation: selection from pointer to "
                                            "interface not allowed"));
            return false;
        }
    }
    
    // TODO: implement
    issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                    issues::Severity::Error,
                                    selection_expr->start(),
                                    "type checking unimplemented"));
    return false;
}

bool ExprHandler::CheckTypeAssertExpr(ast::TypeAssertExpr *type_assert_expr) {
    if (type_assert_expr->type_ == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_assert_expr->start(),
                                        "invalid operation: blank type assertion outside type "
                                        "switch"));
        return false;
    }
    if (!CheckExpr(type_assert_expr->x_.get()) ||
        !CheckExpr(type_assert_expr->type_.get())) {
        return false;
    }
    types::Type *x_type = info_->TypeOf(type_assert_expr->x_.get());
    types::Type *asserted_type = info_->TypeOf(type_assert_expr->type_.get());
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
    
    info_->types_.insert({type_assert_expr, asserted_type});
    info_->expr_kinds_.insert({type_assert_expr, types::ExprKind::kValueOk});
    return true;
}

bool ExprHandler::CheckIndexExpr(ast::IndexExpr *index_expr) {
    if (!CheckExpr(index_expr->accessed_.get()) ||
        !CheckExpr(index_expr->index_.get())) {
        return false;
    }
    
    types::Type *accessed_type = info_->TypeOf(index_expr->accessed_.get());
    types::Type *index_type = info_->TypeOf(index_expr->index_.get());
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
        
        info_->types_.insert({index_expr, element_type});
        info_->expr_kinds_.insert({index_expr, types::ExprKind::kVariable});
        return true;
        
    } else if (auto slice_type = dynamic_cast<types::Array *>(accessed_type->Underlying())) {
        types::Type *element_type = array_type->element_type();
        
        info_->types_.insert({index_expr, element_type});
        info_->expr_kinds_.insert({index_expr, types::ExprKind::kVariable});
        return true;
        
    } else if (auto string_type = dynamic_cast<types::Basic *>(accessed_type->Underlying())) {
        if (!(string_type->info() & types::Basic::Info::kIsString)) {
            add_expected_accessed_value_issue();
            return false;
        }
        
        info_->types_.insert({index_expr, info_->basic_types_.at(types::Basic::Kind::kByte)});
        info_->expr_kinds_.insert({index_expr, types::ExprKind::kValue});
        return true;
        
    } else {
        add_expected_accessed_value_issue();
        return false;
    }
}

bool ExprHandler::CheckCallExpr(ast::CallExpr *call_expr) {
    ast::Expr *func_expr = call_expr->func_.get();
    ast::TypeArgList *type_args_expr = call_expr->type_args_.get();
    std::vector<ast::Expr *> arg_exprs;
    arg_exprs.reserve(call_expr->args_.size());
    for (auto& arg_expr : call_expr->args_) {
        arg_exprs.push_back(arg_expr.get());
    }
    
    if (!CheckExpr(func_expr) ||
        (type_args_expr != nullptr &&
         !TypeHandler::ProcessTypeArgs(type_args_expr, info_, issues_)) ||
        !CheckExprs(arg_exprs)) {
        return false;
    }
    
    if (info_->ExprKindOf(func_expr) == types::ExprKind::kType) {
        return CheckCallExprWithTypeConversion(call_expr,
                                               func_expr,
                                               type_args_expr,
                                               arg_exprs);
    } else if (info_->ExprKindOf(func_expr) == types::ExprKind::kBuiltin) {
        return CheckCallExprWithBuiltin(call_expr,
                                        func_expr,
                                        type_args_expr,
                                        arg_exprs);
    } else if (info_->ExprKindOf(func_expr) == types::ExprKind::kValue ||
               info_->ExprKindOf(func_expr) == types::ExprKind::kVariable) {
        return CheckCallExprWithFuncCall(call_expr,
                                         func_expr,
                                         type_args_expr,
                                         arg_exprs);
    } else {
        throw "internal error: unexpected func expr kind";
    }
}

bool ExprHandler::CheckCallExprWithTypeConversion(ast::CallExpr *call_expr,
                                                  ast::Expr *func_expr,
                                                  ast::TypeArgList *type_args_expr,
                                                  std::vector<ast::Expr *> arg_exprs) {
    if (type_args_expr != nullptr) {
        // TODO: handle this in future
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        call_expr->start(),
                                        "invalid operation: type conversion does not accept "
                                        "type arguments"));
        return false;
    }
    if (arg_exprs.size() != 1) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        call_expr->start(),
                                        "invalid operation: type conversion requires exactly "
                                        "one argument"));
        return false;
    }
    types::Type *conversion_start_type = info_->TypeOf(arg_exprs.at(0));
    types::Type *conversion_result_type = info_->TypeOf(func_expr);
    if (!types::IsConvertibleTo(conversion_start_type, conversion_result_type)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        call_expr->start(),
                                        "invalid operation: type conversion not possible"));
        return false;
    }
    
    info_->types_.insert({call_expr, conversion_result_type});
    info_->expr_kinds_.insert({call_expr, types::ExprKind::kValue});
    return true;
}

bool ExprHandler::CheckCallExprWithBuiltin(ast::CallExpr *call_expr,
                                           ast::Expr *func_expr,
                                           ast::TypeArgList *type_args_expr,
                                           std::vector<ast::Expr *> arg_exprs) {
    // TODO: implement
    throw "interal error: unimplemented";
}

bool ExprHandler::CheckCallExprWithFuncCall(ast::CallExpr *call_expr,
                                            ast::Expr *func_expr,
                                            ast::TypeArgList *type_args_expr,
                                            std::vector<ast::Expr *> arg_exprs) {
    types::Type *func_type = info_->TypeOf(func_expr)->Underlying();
    types::Signature *signature = dynamic_cast<types::Signature *>(func_type);
    if (signature == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        call_expr->start(),
                                        "expected type, function or function variable"));
        return false;
    }
    if (signature->type_parameters() != nullptr) {
        signature = CheckFuncCallTypeArgs(signature, type_args_expr);
        if (signature == nullptr) {
            return false;
        }
    }
    CheckFuncCallArgs(signature, call_expr, arg_exprs);
    CheckFuncCallResultType(signature, call_expr);
    return true;
}

types::Signature * ExprHandler::CheckFuncCallTypeArgs(types::Signature *signature,
                                                      ast::TypeArgList *type_args_expr) {
    size_t given_type_args = (type_args_expr != nullptr) ? type_args_expr->args_.size() : 0;
    size_t expected_type_args = signature->type_parameters()->types().size();
    if (given_type_args != expected_type_args) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        type_args_expr->start(),
                                        "expected " + std::to_string(expected_type_args) +
                                        " type arugments"));
        return nullptr;
    }
    std::unordered_map<types::NamedType *, types::Type *> type_params_to_args;
    for (int i = 0; i < expected_type_args; i++) {
        ast::Expr *type_arg_expr = type_args_expr->args_.at(i).get();
        types::Type *type_arg = info_->TypeOf(type_arg_expr);
        types::NamedType *type_param = signature->type_parameters()->types().at(i);
        
        if (!types::IsAssignableTo(type_arg, type_param)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            type_arg_expr->start(),
                                            "can not assign type argument to parameter"));
            return nullptr;
        }
        type_params_to_args.insert({type_param, type_arg});
    }
    return static_cast<types::Signature *>(types::InstantiateType(signature,
                                                                  type_params_to_args,
                                                                  info_));
}

void ExprHandler::CheckFuncCallArgs(types::Signature *signature,
                                    ast::CallExpr *call_expr,
                                    std::vector<ast::Expr *> arg_exprs) {
    std::vector<types::Type *> arg_types;
    for (ast::Expr *arg_expr : arg_exprs) {
        arg_types.push_back(info_->TypeOf(arg_expr));
    }
    if (arg_types.size() == 1) {
        if (auto tuple = dynamic_cast<types::Tuple *>(arg_types.at(0))) {
            arg_types.clear();
            for (types::Variable *var : tuple->variables()) {
                arg_types.push_back(var->type());
            }
        }
    }
    size_t expected_args = signature->parameters()->variables().size();
    if (arg_types.size() != expected_args) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        call_expr->l_paren_,
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
        info_->expr_kinds_.insert({call_expr, types::ExprKind::kNoValue});
        return;
    }
    
    if (signature->results()->variables().size() == 1) {
        info_->types_.insert({call_expr, signature->results()->variables().at(0)->type()});
        info_->expr_kinds_.insert({call_expr, types::ExprKind::kValue});
        return;
    }
    
    info_->types_.insert({call_expr, signature->results()});
    info_->expr_kinds_.insert({call_expr, types::ExprKind::kValue});
}

bool ExprHandler::CheckFuncLit(ast::FuncLit *func_lit) {
    ast::FuncType *func_type_expr = func_lit->type_.get();
    ast::BlockStmt *func_body = func_lit->body_.get();
    if (!TypeHandler::ProcessTypeExpr(func_type_expr, info_, issues_)) {
        return false;
    }
    types::Signature *func_type = static_cast<types::Signature *>(info_->TypeOf(func_type_expr));
    
    StmtHandler::ProcessFuncBody(func_body, func_type->results(), info_, issues_);
    
    info_->types_.insert({func_lit, func_type});
    info_->expr_kinds_.insert({func_lit, types::ExprKind::kValue});
    return true;
}

bool ExprHandler::CheckCompositeLit(ast::CompositeLit *composite_lit) {
    if (!TypeHandler::ProcessTypeExpr(composite_lit->type_.get(), info_, issues_)) {
        return false;
    }
    types::Type *type = info_->TypeOf(composite_lit->type_.get());
    
    // TODO: check contents of composite lit
    
    info_->types_.insert({composite_lit, type});
    info_->expr_kinds_.insert({composite_lit, types::ExprKind::kValue});
    return true;
}

bool ExprHandler::CheckBasicLit(ast::BasicLit *basic_lit) {
    return ConstantHandler::ProcessConstantExpr(basic_lit, /* iota= */ 0, info_, issues_);
}

bool ExprHandler::CheckIdent(ast::Ident *ident) {
    types::Object *obj = info_->ObjectOf(ident);
    types::ExprKind expr_kind = types::ExprKind::kInvalid;
    if (dynamic_cast<types::TypeName *>(obj) != nullptr) {
        expr_kind = types::ExprKind::kType;
        
    } else if (dynamic_cast<types::Constant *>(obj) != nullptr) {
        expr_kind = types::ExprKind::kConstant;
        
    } else if (dynamic_cast<types::Variable *>(obj) != nullptr) {
        expr_kind = types::ExprKind::kVariable;
        
    } else if (dynamic_cast<types::Func *>(obj) != nullptr ||
               dynamic_cast<types::Nil *>(obj) != nullptr) {
        expr_kind = types::ExprKind::kValue;
        
    } else if (dynamic_cast<types::Builtin *>(obj) != nullptr) {
        expr_kind = types::ExprKind::kBuiltin;
        
    } else if (dynamic_cast<types::PackageName *>(obj) != nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        ident->start(),
                                        "use of package name without selector"));
        return false;
    } else {
        throw "internal error: unexpected object type";
    }
    if (obj->type() == nullptr) {
        // TODO: uncomment exception when stmt handler is fully implemented
        throw "internal error: expect to know type at this point";
        return false;
    }
    
    info_->types_.insert({ident, obj->type()});
    info_->expr_kinds_.insert({ident, expr_kind});
    return true;
}

}
}

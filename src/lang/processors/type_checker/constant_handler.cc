//
//  constant_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "constant_handler.h"

#include "lang/representation/types/types_util.h"

namespace lang {
namespace type_checker {

bool ConstantHandler::ProcessConstant(types::Constant *constant,
                                      types::Type *type,
                                      ast::Expr *value,
                                      int64_t iota,
                                      types::InfoBuilder& info_builder,
                                      std::vector<issues::Issue>& issues) {
    ConstantHandler handler(iota, info_builder, issues);
    
    return handler.ProcessConstantDefinition(constant, type, value);
}

bool ConstantHandler::ProcessConstantExpr(ast::Expr *constant_expr,
                                          int64_t iota,
                                          types::InfoBuilder& info_builder,
                                          std::vector<issues::Issue>& issues) {
    ConstantHandler handler(iota, info_builder, issues);
    
    return handler.EvaluateConstantExpr(constant_expr);
}

bool ConstantHandler::ProcessConstantDefinition(types::Constant *constant,
                                                types::Type *type,
                                                ast::Expr *value_expr) {
    if (type == nullptr && value_expr == nullptr) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        constant->position(),
                                        "constant needs a type or value: "
                                        + constant->name()));
        return false;
    }
    
    types::Basic *basic_type = nullptr;
    constants::Value value(int64_t{0});
    
    if (type != nullptr) {
        types::Type *underlying = types::UnderlyingOf(type);
        if (underlying == nullptr ||
            underlying->type_kind() != types::TypeKind::kBasic) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            constant->position(),
                                            "constant can not have non-basic type: "
                                            + constant->name()));
            return false;
        }
        basic_type = static_cast<types::Basic *>(underlying);
    }
    
    if (value_expr == nullptr) {
        value = ConvertUntypedInt(value, basic_type->kind());
        
    } else {
        if (!EvaluateConstantExpr(value_expr)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            constant->position(),
                                            "constant could not be evaluated: "
                                            + constant->name()));
            return false;
        }
        types::ExprInfo value_expr_info = info_->ExprInfoOf(value_expr).value();
        types::Basic *given_type = static_cast<types::Basic *>(value_expr_info.type());
        constants::Value given_value = info_->constant_values().at(value_expr);
        
        if (basic_type == nullptr) {
            type = given_type;
            basic_type = given_type;
        }
        
        if (given_type == basic_type) {
            value = given_value;
            
        } else if (given_type->info() & types::Basic::kIsUntyped) {
            value = ConvertUntypedInt(given_value, basic_type->kind());
            
        } else {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            constant->position(),
                                            "constant can not hold a value of a different type: "
                                            + constant->name()));
            return false;
        }
    }
    
    info_builder_.SetObjectType(constant, type);
    info_builder_.SetConstantValue(constant, value);
    return true;
}

bool ConstantHandler::EvaluateConstantExpr(ast::Expr *expr) {
    switch (expr->node_kind()) {
        case ast::NodeKind::kIdent:{
            ast::Ident *ident = static_cast<ast::Ident *>(expr);
            types::Constant *constant = static_cast<types::Constant *>(info_->uses().at(ident));
            types::Type *type = constant->type();
            constants::Value value(0);
            if (constant == nullptr) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                ident->start(),
                                                "constant can not depend on unknown ident: "
                                                + ident->name()));
                return false;
            } else if (constant->parent() == info_->universe() &&
                       constant->name() == "iota") {
                value = constants::Value(iota_);
            } else {
                value = constant->value();
            }
            
            info_builder_.SetExprInfo(expr, types::ExprInfo(types::ExprInfo::Kind::kConstant,
                                                            type));
            info_builder_.SetExprConstantValue(expr, value);
            return true;
        }
        case ast::NodeKind::kBasicLit:{
            ast::BasicLit *basic_lit = static_cast<ast::BasicLit *>(expr);
            types::Basic *type;
            constants::Value value(0);
            switch (basic_lit->kind()) {
                case tokens::kInt:
                    type = info_->basic_type(types::Basic::kUntypedInt);
                    value = constants::Value(std::stoull(basic_lit->value()));
                    break;
                case tokens::kChar:
                    // TODO: support UTF-8 and character literals
                    type = info_->basic_type(types::Basic::kUntypedRune);
                    value = constants::Value(int32_t(basic_lit->value().at(1)));
                    break;
                case tokens::kString:
                    type = info_->basic_type(types::Basic::kUntypedString);
                    value = constants::Value(basic_lit->value().substr(1,
                                                                       basic_lit->value().length() - 2));
                    break;
                default:
                    throw "internal error: unexpected basic literal kind";
            }
            
            info_builder_.SetExprInfo(expr, types::ExprInfo(types::ExprInfo::Kind::kConstant,
                                                            type));
            info_builder_.SetExprConstantValue(expr, value);
            return true;
        }
        case ast::NodeKind::kParenExpr:{
            ast::ParenExpr *paren_expr = static_cast<ast::ParenExpr *>(expr);
            if (!EvaluateConstantExpr(paren_expr->x())) {
                return false;
            }
            types::ExprInfo x_info = info_->ExprInfoOf(paren_expr->x()).value();
            constants::Value value = info_->constant_values().at(paren_expr->x());
            
            info_builder_.SetExprInfo(expr, x_info);
            info_builder_.SetExprConstantValue(expr, value);
            return true;
        }
        case ast::NodeKind::kUnaryExpr:
            return EvaluateConstantUnaryExpr(static_cast<ast::UnaryExpr *>(expr));
        case ast::NodeKind::kBinaryExpr:{
            ast::BinaryExpr *binary_expr = static_cast<ast::BinaryExpr *>(expr);
            switch (binary_expr->op()) {
                case tokens::kLss:
                case tokens::kLeq:
                case tokens::kGeq:
                case tokens::kGtr:
                case tokens::kEql:
                case tokens::kNeq:
                    return EvaluateConstantCompareExpr(binary_expr);
                case tokens::kShl:
                case tokens::kShr:
                    return EvaluateConstantShiftExpr(binary_expr);
                default:
                    return EvaluateConstantBinaryExpr(binary_expr);
            }
        }
        default:
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            expr->start(),
                                            "constant expression not allowed"));
            return false;
    }
}

bool ConstantHandler::EvaluateConstantUnaryExpr(ast::UnaryExpr *expr) {
    if (!EvaluateConstantExpr(expr->x())) {
        return false;
    }
    types::ExprInfo x_info = info_->ExprInfoOf(expr->x()).value();
    types::Basic *x_type = static_cast<types::Basic *>(x_info.type());
    constants::Value x_value = info_->constant_values().at(expr->x());
    
    switch (x_type->kind()) {
        case types::Basic::kBool:
        case types::Basic::kUntypedBool:
            if (expr->op() != tokens::kNot) {
                issues_.push_back(
                                  issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "unary operator not allowed for constant expression"));
                return false;
            }
            info_builder_.SetExprInfo(expr, types::ExprInfo(types::ExprInfo::Kind::kConstant,
                                                            x_type));
            info_builder_.SetExprConstantValue(expr, constants::UnaryOp(expr->op(), x_value));
            return true;
            
        case types::Basic::kUint:
        case types::Basic::kUint8:
        case types::Basic::kUint16:
        case types::Basic::kUint32:
        case types::Basic::kUint64:
        case types::Basic::kInt:
        case types::Basic::kInt8:
        case types::Basic::kInt16:
        case types::Basic::kInt32:
        case types::Basic::kInt64:
        case types::Basic::kUntypedInt:
        case types::Basic::kUntypedRune:{
            if (expr->op() != tokens::kAdd &&
                expr->op() != tokens::kSub &&
                expr->op() != tokens::kXor) {
                issues_.push_back(
                                  issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "unary operator not allowed for constant expression"));
                return false;
            }
            types::Basic *result_type = x_type;
            if (expr->op() == tokens::kSub &&
                result_type->info() & types::Basic::kIsUnsigned) {
                constexpr types::Basic::Kind diff{types::Basic::kUint - types::Basic::kInt};
                result_type =
                    info_->basic_type(types::Basic::Kind(result_type->kind() - diff));
            }
            
            info_builder_.SetExprInfo(expr, types::ExprInfo(types::ExprInfo::Kind::kConstant,
                                                            result_type));
            info_builder_.SetExprConstantValue(expr, constants::UnaryOp(expr->op(), x_value));
            return true;
        }
        case types::Basic::kString:
        case types::Basic::kUntypedString:
            issues_.push_back(
                              issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            expr->start(),
                                            "unary operator not allowed for constant expression"));
            return false;
        default:
            throw "unexpected type";
    }
}

bool ConstantHandler::EvaluateConstantCompareExpr(ast::BinaryExpr *expr) {
    if (!EvaluateConstantExpr(expr->x())) {
        return false;
    }
    if (!EvaluateConstantExpr(expr->y())) {
        return false;
    }
    types::ExprInfo x_info = info_->ExprInfoOf(expr->x()).value();
    types::ExprInfo y_info = info_->ExprInfoOf(expr->y()).value();
    types::Basic *x_type = static_cast<types::Basic *>(x_info.type());
    types::Basic *y_type = static_cast<types::Basic *>(y_info.type());
    switch (expr->op()) {
        case tokens::kLss:
        case tokens::kLeq:
        case tokens::kGeq:
        case tokens::kGtr:
            if ((x_type->info() & types::Basic::kIsOrdered) == 0 ||
                (y_type->info() & types::Basic::kIsOrdered) == 0) {
                issues_.push_back(
                                  issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "comparison of constant expressions with given types not "
                                                "allowed"));
                return false;
            }
        default:;
    }
    
    constants::Value x_value = info_->constant_values().at(expr->x());
    constants::Value y_value = info_->constant_values().at(expr->y());
    types::Basic *result_type;
    
    if (!CheckTypesForRegualarConstantBinaryExpr(expr,
                                                 x_value,
                                                 y_value,
                                                 result_type)) {
        return false;
    }
    
    bool result = constants::Compare(x_value, expr->op(), y_value);
    
    info_builder_.SetExprInfo(expr, types::ExprInfo(types::ExprInfo::Kind::kConstant,
                                                    result_type));
    info_builder_.SetExprConstantValue(expr, constants::Value(result));
    return true;
}

bool ConstantHandler::EvaluateConstantShiftExpr(ast::BinaryExpr *expr) {
    if (!EvaluateConstantExpr(expr->x())) {
        return false;
    }
    if (!EvaluateConstantExpr(expr->y())) {
        return false;
    }
    types::ExprInfo x_info = info_->ExprInfoOf(expr->x()).value();
    types::ExprInfo y_info = info_->ExprInfoOf(expr->y()).value();
    types::Basic *x_type = static_cast<types::Basic *>(x_info.type());
    types::Basic *y_type = static_cast<types::Basic *>(y_info.type());
    
    if ((x_type->info() & types::Basic::kIsNumeric) == 0 ||
        (y_type->info() & types::Basic::kIsNumeric) == 0) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        expr->start(),
                                        "constant shift expressions with given types not allowed"));
        return false;
    }
    
    constants::Value x_value = info_->constant_values().at(expr->x());
    constants::Value y_value = info_->constant_values().at(expr->y());
    
    if ((x_type->info() & types::Basic::kIsUntyped) != 0) {
        x_value = ConvertUntypedInt(x_value, types::Basic::kInt);
    }
    if ((y_type->info() & types::Basic::kIsUntyped) != 0) {
        y_value = ConvertUntypedInt(y_value, types::Basic::kUint);
    } else if ((y_type->info() & types::Basic::kIsUnsigned) == 0) {
        issues_.push_back(
                          issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        expr->start(),
                                        "constant shift expressions with signed shift operand not allowed"));
        return false;
    }
    
    info_builder_.SetExprInfo(expr, types::ExprInfo(types::ExprInfo::Kind::kConstant,
                                                    x_type));
    info_builder_.SetExprConstantValue(expr, constants::ShiftOp(x_value, expr->op(), y_value));
    return true;
}

bool ConstantHandler::EvaluateConstantBinaryExpr(ast::BinaryExpr *expr) {
    if (!EvaluateConstantExpr(expr->x())) {
        return false;
    }
    if (!EvaluateConstantExpr(expr->y())) {
        return false;
    }
    types::ExprInfo x_info = info_->ExprInfoOf(expr->x()).value();
    types::ExprInfo y_info = info_->ExprInfoOf(expr->y()).value();
    types::Basic *x_type = static_cast<types::Basic *>(x_info.type());
    types::Basic *y_type = static_cast<types::Basic *>(y_info.type());
    switch (expr->op()) {
        case tokens::kLAnd:
        case tokens::kLOr:
            if ((x_type->info() & types::Basic::kIsBoolean) == 0 ||
                (y_type->info() & types::Basic::kIsBoolean) == 0) {
                issues_.push_back(
                                  issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "binary operation with constant expressions of given types not "
                                                "allowed"));
                return false;
            }
            break;
        case tokens::kAdd:
            if ((x_type->info() & types::Basic::kIsString) &&
                (y_type->info() & types::Basic::kIsString)) {
                break;
            }
        case tokens::kSub:
        case tokens::kMul:
        case tokens::kQuo:
        case tokens::kRem:
        case tokens::kAnd:
        case tokens::kOr:
        case tokens::kXor:
        case tokens::kAndNot:
            if ((x_type->info() & types::Basic::kIsNumeric) == 0 ||
                (y_type->info() & types::Basic::kIsNumeric) == 0) {
                issues_.push_back(
                                  issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "binary operation with constant expressions of given types not "
                                                "allowed"));
                return false;
            }
            break;
        default:;
    }
    
    constants::Value x_value = info_->constant_values().at(expr->x());
    constants::Value y_value = info_->constant_values().at(expr->y());
    types::Basic *result_type;
    
    if (!CheckTypesForRegualarConstantBinaryExpr(expr,
                                                 x_value,
                                                 y_value,
                                                 result_type)) {
        return false;
    }
    
    info_builder_.SetExprInfo(expr, types::ExprInfo(types::ExprInfo::Kind::kConstant,
                                                    result_type));
    info_builder_.SetExprConstantValue(expr, constants::BinaryOp(x_value, expr->op(), y_value));
    return true;
}


bool ConstantHandler::CheckTypesForRegualarConstantBinaryExpr(ast::BinaryExpr *expr,
                                                              constants::Value &x_value,
                                                              constants::Value &y_value,
                                                              types::Basic* &result_type) {
    types::ExprInfo x_info = info_->ExprInfoOf(expr->x()).value();
    types::ExprInfo y_info = info_->ExprInfoOf(expr->y()).value();
    types::Basic *x_type = static_cast<types::Basic *>(x_info.type());
    types::Basic *y_type = static_cast<types::Basic *>(y_info.type());
    
    if (!((x_type->info() & types::Basic::kIsBoolean) &&
          (y_type->info() & types::Basic::kIsBoolean)) ||
        !((x_type->info() & types::Basic::kIsNumeric) &&
          (y_type->info() & types::Basic::kIsNumeric)) ||
        !((x_type->info() & types::Basic::kIsString) &&
          (y_type->info() & types::Basic::kIsString))) {
        issues_.push_back(
                          issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        expr->start(),
                                        "comparison of constant expressions with given types not allowed"));
        return false;
    }
    
    if ((x_type->info() & types::Basic::kIsInteger) != 0) {
        if ((x_type->info() & types::Basic::kIsUntyped) != 0 &&
            (y_type->info() & types::Basic::kIsUntyped) != 0) {
            x_value = ConvertUntypedInt(x_value, types::Basic::kInt);
            y_value = ConvertUntypedInt(y_value, types::Basic::kInt);
            result_type = x_type;
        } else if ((x_type->info() & types::Basic::kIsUntyped) != 0) {
            x_value = ConvertUntypedInt(x_value, y_type->kind());
            result_type = y_type;
        } else if ((y_type->info() & types::Basic::kIsUntyped) != 0) {
            y_value = ConvertUntypedInt(y_value, x_type->kind());
            result_type = x_type;
        } else {
            if (x_type->kind() != y_type->kind()) {
                issues_.push_back(
                                  issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "comparison of constant expressions of different types not "
                                                "allowed"));
                return false;
            }
            result_type = x_type;
        }
    } else if ((x_type->info() & types::Basic::kIsBoolean) != 0) {
        if ((x_type->info() & types::Basic::kIsUntyped) != 0 &&
            (y_type->info() & types::Basic::kIsUntyped) != 0) {
            result_type = info_->basic_type(types::Basic::kUntypedBool);
        } else {
            result_type = info_->basic_type(types::Basic::kBool);
        }
    } else if ((x_type->info() & types::Basic::kIsString) != 0) {
        if ((x_type->info() & types::Basic::kIsUntyped) != 0 &&
            (y_type->info() & types::Basic::kIsUntyped) != 0) {
            result_type = info_->basic_type(types::Basic::kUntypedString);
        } else {
            result_type = info_->basic_type(types::Basic::kString);
        }
    }
    
    else {
        throw "internal error";
    }
    return true;
}

constants::Value ConstantHandler::ConvertUntypedInt(constants::Value value, types::Basic::Kind kind) {
    switch (kind) {
        case types::Basic::kInt8:
            switch (value.value_.index()) {
                case 7:
                    return constants::Value(int8_t(std::get<int64_t>(value.value_)));
                case 8:
                    return constants::Value(int8_t(std::get<uint64_t>(value.value_)));
            }
        case types::Basic::kInt16:
            switch (value.value_.index()) {
                case 7:
                    return constants::Value(int16_t(std::get<int64_t>(value.value_)));
                case 8:
                    return constants::Value(int16_t(std::get<uint64_t>(value.value_)));
            }
        case types::Basic::kInt32:
            switch (value.value_.index()) {
                case 7:
                    return constants::Value(int32_t(std::get<int64_t>(value.value_)));
                case 8:
                    return constants::Value(int32_t(std::get<uint64_t>(value.value_)));
            }
        case types::Basic::kInt64:
        case types::Basic::kInt:
            switch (value.value_.index()) {
                case 7:
                    return constants::Value(int64_t(std::get<int64_t>(value.value_)));
                case 8:
                    return constants::Value(int64_t(std::get<uint64_t>(value.value_)));
            }
        case types::Basic::kUint8:
            switch (value.value_.index()) {
                case 7:
                    return constants::Value(uint8_t(std::get<int64_t>(value.value_)));
                case 8:
                    return constants::Value(uint8_t(std::get<uint64_t>(value.value_)));
            }
        case types::Basic::kUint16:
            switch (value.value_.index()) {
                case 7:
                    return constants::Value(uint16_t(std::get<int64_t>(value.value_)));
                case 8:
                    return constants::Value(uint16_t(std::get<uint64_t>(value.value_)));
            }
        case types::Basic::kUint32:
            switch (value.value_.index()) {
                case 7:
                    return constants::Value(uint32_t(std::get<int64_t>(value.value_)));
                case 8:
                    return constants::Value(uint32_t(std::get<uint64_t>(value.value_)));
            }
        case types::Basic::kUint64:
        case types::Basic::kUint:
            switch (value.value_.index()) {
                case 7:
                    return constants::Value(uint64_t(std::get<int64_t>(value.value_)));
                case 8:
                    return constants::Value(uint64_t(std::get<uint64_t>(value.value_)));
            }
        default:;
    }
    throw "internal error";
}

}
}

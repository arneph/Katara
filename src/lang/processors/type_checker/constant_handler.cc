//
//  constant_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "constant_handler.h"

namespace lang {
namespace type_checker {

bool ConstantHandler::ProcessConstant(types::Constant *constant,
                                      types::Type *type,
                                      ast::Expr *value,
                                      int64_t iota,
                                      types::TypeInfo *info,
                                      std::vector<issues::Issue>& issues) {
    ConstantHandler handler(iota, info, issues);
    
    return handler.ProcessConstantDefinition(constant, type, value);
}

bool ConstantHandler::ProcessConstantExpr(ast::Expr *constant_expr,
                                          int64_t iota,
                                          types::TypeInfo *info,
                                          std::vector<issues::Issue>& issues) {
    ConstantHandler handler(iota, info, issues);
    
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
        basic_type = dynamic_cast<types::Basic *>(type->Underlying());
        if (type == nullptr) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            constant->position(),
                                            "constant can not have non-basic type: "
                                            + constant->name()));
            return false;
        }
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
        types::Basic *given_type = static_cast<types::Basic *>(info_->types_.at(value_expr));
        constants::Value given_value = info_->constant_values_.at(value_expr);
        
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
    
    constant->type_ = type;
    constant->value_ = value;
    return true;
}

bool ConstantHandler::EvaluateConstantExpr(ast::Expr *expr) {
    if (ast::Ident *ident = dynamic_cast<ast::Ident *>(expr)) {
        types::Constant *constant = dynamic_cast<types::Constant *>(info_->uses().at(ident));
        types::Type *type = constant->type_;
        constants::Value value(0);
        if (constant == nullptr) {
            issues_.push_back(
                              issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            ident->start(),
                                            "constant can not depend on unknown ident: "
                                            + ident->name_));
            return false;
        } else if (constant->parent_ == info_->universe_ &&
                   constant->name_ == "iota") {
            value = constants::Value(iota_);
        } else {
            value = constant->value_;
        }
        
        info_->types_.insert({expr, type});
        info_->expr_kinds_.insert({expr, types::ExprKind::kConstant});
        info_->constant_values_.insert({expr, value});
        return true;
        
    } else if (ast::BasicLit *basic_lit = dynamic_cast<ast::BasicLit *>(expr)) {
        uint64_t v = std::stoull(basic_lit->value_);
        
        // TODO: support strings and other basic literals
        types::Basic *type = info_->basic_types_.at(types::Basic::kUntypedInt);
        constants::Value value(v);
        
        info_->types_.insert({expr, type});
        info_->expr_kinds_.insert({expr, types::ExprKind::kConstant});
        info_->constant_values_.insert({expr, value});
        return true;
        
    } else if (ast::ParenExpr *paren_expr = dynamic_cast<ast::ParenExpr *>(expr)) {
        if (!EvaluateConstantExpr(paren_expr->x_.get())) {
            return false;
        }
        
        types::Type *type = info_->types_.at(paren_expr->x_.get());
        constants::Value value = info_->constant_values_.at(paren_expr->x_.get());
        
        info_->types_.insert({expr, type});
        info_->expr_kinds_.insert({expr, types::ExprKind::kConstant});
        info_->constant_values_.insert({expr, value});
        return true;
        
    } else if (ast::UnaryExpr *unary_expr = dynamic_cast<ast::UnaryExpr *>(expr)) {
        return EvaluateConstantUnaryExpr(unary_expr);
        
    } else if (ast::BinaryExpr *binary_expr = dynamic_cast<ast::BinaryExpr *>(expr)) {
        switch (binary_expr->op_) {
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
    } else {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        expr->start(),
                                        "constant expression not allowed"));
        return false;
    }
}

bool ConstantHandler::EvaluateConstantUnaryExpr(ast::UnaryExpr *expr) {
    if (!EvaluateConstantExpr(expr->x_.get())) {
        return false;
    }
    
    types::Basic *x_type = static_cast<types::Basic *>(info_->types_.at(expr->x_.get()));
    constants::Value x_value = info_->constant_values_.at(expr->x_.get());
    
    switch (x_type->kind()) {
        case types::Basic::kBool:
        case types::Basic::kUntypedBool:
            if (expr->op_ != tokens::kNot) {
                issues_.push_back(
                                  issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "unary operator not allowed for constant expression"));
                return false;
            }
            info_->types_.insert({expr, x_type});
            info_->expr_kinds_.insert({expr, types::ExprKind::kConstant});
            info_->constant_values_.insert({expr, constants::UnaryOp(expr->op_, x_value)});
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
            if (expr->op_ != tokens::kAdd &&
                expr->op_ != tokens::kSub &&
                expr->op_ != tokens::kXor) {
                issues_.push_back(
                                  issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "unary operator not allowed for constant expression"));
                return false;
            }
            types::Basic *result_type = x_type;
            if (expr->op_ == tokens::kSub &&
                result_type->info() & types::Basic::kIsUnsigned) {
                constexpr types::Basic::Kind diff{types::Basic::kUint - types::Basic::kInt};
                result_type =
                    info_->basic_types_.at(types::Basic::Kind(result_type->kind() - diff));
            }
            
            info_->types_.insert({expr, result_type});
            info_->expr_kinds_.insert({expr, types::ExprKind::kConstant});
            info_->constant_values_.insert({expr, constants::UnaryOp(expr->op_, x_value)});
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
    if (!EvaluateConstantExpr(expr->x_.get())) {
        return false;
    }
    if (!EvaluateConstantExpr(expr->y_.get())) {
        return false;
    }
    
    types::Basic *x_type = static_cast<types::Basic *>(info_->types_.at(expr->x_.get()));
    types::Basic *y_type = static_cast<types::Basic *>(info_->types_.at(expr->y_.get()));
    switch (expr->op_) {
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
    
    constants::Value x_value = info_->constant_values_.at(expr->x_.get());
    constants::Value y_value = info_->constant_values_.at(expr->y_.get());
    types::Basic *result_type;
    
    if (!CheckTypesForRegualarConstantBinaryExpr(expr,
                                                 x_value,
                                                 y_value,
                                                 result_type)) {
        return false;
    }
    
    bool result = constants::Compare(x_value, expr->op_, y_value);
    
    info_->types_.insert({expr, result_type});
    info_->expr_kinds_.insert({expr, types::ExprKind::kConstant});
    info_->constant_values_.insert({expr, constants::Value(result)});
    return true;
}

bool ConstantHandler::EvaluateConstantShiftExpr(ast::BinaryExpr *expr) {
    if (!EvaluateConstantExpr(expr->x_.get())) {
        return false;
    }
    if (!EvaluateConstantExpr(expr->y_.get())) {
        return false;
    }
    
    types::Basic *x_type = static_cast<types::Basic *>(info_->types_.at(expr->x_.get()));
    types::Basic *y_type = static_cast<types::Basic *>(info_->types_.at(expr->y_.get()));
    
    if ((x_type->info() & types::Basic::kIsNumeric) == 0 ||
        (y_type->info() & types::Basic::kIsNumeric) == 0) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        expr->start(),
                                        "constant shift expressions with given types not allowed"));
        return false;
    }
    
    constants::Value x_value = info_->constant_values_.at(expr->x_.get());
    constants::Value y_value = info_->constant_values_.at(expr->y_.get());
    
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
    
    info_->types_.insert({expr, x_type});
    info_->expr_kinds_.insert({expr, types::ExprKind::kConstant});
    info_->constant_values_.insert({expr, constants::ShiftOp(x_value, expr->op_, y_value)});
    return true;
}

bool ConstantHandler::EvaluateConstantBinaryExpr(ast::BinaryExpr *expr) {
    if (!EvaluateConstantExpr(expr->x_.get())) {
        return false;
    }
    if (!EvaluateConstantExpr(expr->y_.get())) {
        return false;
    }
    
    types::Basic *x_type = static_cast<types::Basic *>(info_->types_.at(expr->x_.get()));
    types::Basic *y_type = static_cast<types::Basic *>(info_->types_.at(expr->y_.get()));
    switch (expr->op_) {
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
    
    constants::Value x_value = info_->constant_values_.at(expr->x_.get());
    constants::Value y_value = info_->constant_values_.at(expr->y_.get());
    types::Basic *result_type;
    
    if (!CheckTypesForRegualarConstantBinaryExpr(expr,
                                                 x_value,
                                                 y_value,
                                                 result_type)) {
        return false;
    }
    
    info_->types_.insert({expr, result_type});
    info_->expr_kinds_.insert({expr, types::ExprKind::kConstant});
    info_->constant_values_.insert({expr, constants::BinaryOp(x_value, expr->op_, y_value)});
    return true;
}


bool ConstantHandler::CheckTypesForRegualarConstantBinaryExpr(ast::BinaryExpr *expr,
                                                          constants::Value &x_value,
                                                          constants::Value &y_value,
                                                          types::Basic* &result_type) {
    types::Basic *x_type = static_cast<types::Basic *>(info_->types_.at(expr->x_.get()));
    types::Basic *y_type = static_cast<types::Basic *>(info_->types_.at(expr->y_.get()));
    
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
            result_type = info_->basic_types_.at(types::Basic::kUntypedBool);
        } else {
            result_type = info_->basic_types_.at(types::Basic::kBool);
        }
    } else if ((x_type->info() & types::Basic::kIsString) != 0) {
        if ((x_type->info() & types::Basic::kIsUntyped) != 0 &&
            (y_type->info() & types::Basic::kIsUntyped) != 0) {
            result_type = info_->basic_types_.at(types::Basic::kUntypedString);
        } else {
            result_type = info_->basic_types_.at(types::Basic::kString);
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

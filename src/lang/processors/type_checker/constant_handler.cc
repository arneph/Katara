//
//  constant_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "constant_handler.h"

#include "lang/representation/ast/ast_util.h"

namespace lang {
namespace type_checker {

void ConstantHandler::HandleConstants(std::vector<ast::File *> package_files,
                                      types::Package *package,
                                      types::TypeInfo *info,
                                      std::vector<issues::Issue> &issues) {
    ConstantHandler handler(package_files,
                            package,
                            info,
                            issues);
    
    handler.EvaluateConstants();
}

void ConstantHandler::EvaluateConstants() {
    std::vector<EvalInfo> eval_infos = FindConstantEvaluationInfo();
    eval_infos = FindConstantsEvaluationOrder(eval_infos);
    
    for (auto& eval_info : eval_infos) {
        EvaluateConstant(eval_info);
    }
}

std::vector<ConstantHandler::EvalInfo>
ConstantHandler::FindConstantsEvaluationOrder(std::vector<EvalInfo> eval_infos) {
    std::vector<ConstantHandler::EvalInfo> order;
    std::unordered_set<types::Constant *> done;
    while (eval_infos.size() > done.size()) {
        size_t done_size_before = done.size();
        
        for (auto& info : eval_infos) {
            if (done.find(info.constant_) != done.end()) {
                continue;
            }
            bool all_dependencies_done = true;
            for (auto dependency : info.dependencies_) {
                if (done.find(dependency) == done.end()) {
                    all_dependencies_done = false;
                    break;
                }
            }
            if (!all_dependencies_done) {
                continue;
            }
            
            order.push_back(info);
            done.insert(info.constant_);
        }
        
        size_t done_size_after = done.size();
        if (done_size_before == done_size_after) {
            std::vector<pos::pos_t> positions;
            std::string names = "";
            for (auto& info : eval_infos) {
                positions.push_back(info.constant_->position_);
                if (names.empty()) {
                    names = info.constant_->name_;
                } else {
                    names += ", " + info.constant_->name_;
                }
            }
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            positions,
                                            "initialization loop(s) for constants: " + names));
            break;
        }
    }
    return order;
}

std::vector<ConstantHandler::EvalInfo>
ConstantHandler::FindConstantEvaluationInfo() {
    std::vector<EvalInfo> eval_info;
    for (ast::File *file : package_files_) {
        for (auto& decl : file->decls_) {
            auto gen_decl = dynamic_cast<ast::GenDecl *>(decl.get());
            if (gen_decl == nullptr ||
                gen_decl->tok_ != tokens::kConst) {
                continue;
            }
            int64_t iota = 0;
            for (auto& spec : gen_decl->specs_) {
                auto value_spec = static_cast<ast::ValueSpec *>(spec.get());
                
                for (size_t i = 0; i < value_spec->names_.size(); i++) {
                    ast::Ident *name = value_spec->names_.at(i).get();
                    types::Object *object = info_->definitions_.at(name);
                    types::Constant *constant = static_cast<types::Constant *>(object);
                    ast::Expr *type = value_spec->type_.get();
                    ast::Expr *value = nullptr;
                    std::unordered_set<types::Constant *> dependencies;
                    if (value_spec->values_.size() > i) {
                        value = value_spec->values_.at(i).get();
                        dependencies = FindConstantDependencies(value);
                    }
                    
                    eval_info.push_back(EvalInfo{
                        constant, name, type, value, iota, dependencies
                    });
                }
                iota++;
            }
        }
    }
    return eval_info;
}

std::unordered_set<types::Constant *> ConstantHandler::FindConstantDependencies(ast::Expr *expr) {
    std::unordered_set<types::Constant *> constants;
    ast::WalkFunction walker =
    ast::WalkFunction([&](ast::Node *node) -> ast::WalkFunction {
        if (node == nullptr) {
            return walker;
        }
        auto ident = dynamic_cast<ast::Ident *>(node);
        if (ident == nullptr) {
            return walker;
        }
        auto it = info_->uses_.find(ident);
        if (it == info_->uses_.end() ||
            it->second->parent() != package_->scope() ||
            (dynamic_cast<types::Constant *>(it->second) == nullptr &&
             dynamic_cast<types::Variable *>(it->second) == nullptr &&
             dynamic_cast<types::Func *>(it->second) == nullptr)) {
            return walker;
        }
        auto constant = dynamic_cast<types::Constant *>(it->second);
        if (constant == nullptr) {
            issues_.push_back(
                              issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            ident->start(),
                                            "constant can not depend on non-constant: " + ident->name_));
            return walker;
        }
        constants.insert(constant);
        return walker;
    });
    ast::Walk(expr, walker);
    return constants;
}

void ConstantHandler::EvaluateConstant(EvalInfo &eval_info) {
    types::Basic *type = nullptr;
    constants::Value value(int64_t{0});
    if (eval_info.value_ == nullptr) {
        if (eval_info.type_ == nullptr) {
            issues_.push_back(
                              issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            eval_info.name_->start(),
                                            "constant needs a type or value: " +
                                                eval_info.name_->name_));
            return;
        }
        type = dynamic_cast<types::Basic *>(info_->types_.at(eval_info.type_));
        if (type == nullptr) {
            issues_.push_back(
                              issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            eval_info.name_->start(),
                                            "constant can not have non-basic type: " + eval_info.name_->name_));
            return;
        }
        value = ConvertUntypedInt(value, type->kind());
        
    } else {
        if (!EvaluateConstantExpr(eval_info.value_, eval_info.iota_)) {
            issues_.push_back(
                              issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            eval_info.name_->start(),
                                            "constant could not be evaluated: " + eval_info.name_->name_));
            return;
        }
    }
}

bool ConstantHandler::EvaluateConstantExpr(ast::Expr *expr, int64_t iota) {
    if (ast::Ident *ident = dynamic_cast<ast::Ident *>(expr)) {
        types::Constant *constant = dynamic_cast<types::Constant *>(info_->uses().at(ident));
        types::Basic *type = static_cast<types::Basic *>(constant->type_);
        constants::Value value(0);
        if (constant == nullptr) {
            issues_.push_back(
                              issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            ident->start(),
                                            "constant can not depend on unknown ident: " + ident->name_));
            return false;
        } else if (constant->parent_ == info_->universe_ &&
                   constant->name_ == "iota") {
            value = constants::Value(iota);
        } else {
            value = constant->value_;
        }
        
        info_->types_.insert({expr, type});
        info_->constant_values_.insert({expr, value});
        return true;
        
    } else if (ast::BasicLit *basic_lit = dynamic_cast<ast::BasicLit *>(expr)) {
        uint64_t v = std::stoull(basic_lit->value_);
        
        types::Basic *type = info_->basic_types_.at(types::Basic::kUntypedInt);
        constants::Value value(v);
        
        info_->types_.insert({expr, type});
        info_->constant_values_.insert({expr, value});
        return true;
        
    } else if (ast::ParenExpr *paren_expr = dynamic_cast<ast::ParenExpr *>(expr)) {
        if (!EvaluateConstantExpr(paren_expr->x_.get(), iota)) {
            return false;
        }
        
        types::Type *type = info_->types_.at(paren_expr->x_.get());
        constants::Value value = info_->constant_values_.at(paren_expr->x_.get());
        
        info_->types_.insert({expr, type});
        info_->constant_values_.insert({expr, value});
        return true;
        
    } else if (ast::UnaryExpr *unary_expr = dynamic_cast<ast::UnaryExpr *>(expr)) {
        return EvaluateConstantUnaryExpr(unary_expr, iota);
        
    } else if (ast::BinaryExpr *binary_expr = dynamic_cast<ast::BinaryExpr *>(expr)) {
        switch (binary_expr->op_) {
            case tokens::kLss:
            case tokens::kLeq:
            case tokens::kGeq:
            case tokens::kGtr:
            case tokens::kEql:
            case tokens::kNeq:
                return EvaluateConstantCompareExpr(binary_expr, iota);
            case tokens::kShl:
            case tokens::kShr:
                return EvaluateConstantShiftExpr(binary_expr, iota);
            default:
                return EvaluateConstantBinaryExpr(binary_expr, iota);
        }
    } else {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        expr->start(),
                                        "constant expression not allowed"));
        return false;
    }
}

bool ConstantHandler::EvaluateConstantUnaryExpr(ast::UnaryExpr *expr, int64_t iota) {
    if (!EvaluateConstantExpr(expr->x_.get(), iota)) {
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

bool ConstantHandler::EvaluateConstantCompareExpr(ast::BinaryExpr *expr, int64_t iota) {
    if (!EvaluateConstantExpr(expr->x_.get(), iota)) {
        return false;
    }
    if (!EvaluateConstantExpr(expr->y_.get(), iota)) {
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
    info_->constant_values_.insert({expr, constants::Value(result)});
    return true;
}

bool ConstantHandler::EvaluateConstantShiftExpr(ast::BinaryExpr *expr, int64_t iota) {
    if (!EvaluateConstantExpr(expr->x_.get(), iota)) {
        return false;
    }
    if (!EvaluateConstantExpr(expr->y_.get(), iota)) {
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
    info_->constant_values_.insert({expr, constants::ShiftOp(x_value, expr->op_, y_value)});
    return true;
}

bool ConstantHandler::EvaluateConstantBinaryExpr(ast::BinaryExpr *expr, int64_t iota) {
    if (!EvaluateConstantExpr(expr->x_.get(), iota)) {
        return false;
    }
    if (!EvaluateConstantExpr(expr->y_.get(), iota)) {
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

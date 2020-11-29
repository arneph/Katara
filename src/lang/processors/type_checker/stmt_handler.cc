//
//  stmt_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 11/22/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "stmt_handler.h"

#include "lang/representation/ast/ast_util.h"
#include "lang/representation/types/types_util.h"
#include "lang/processors/type_checker/type_handler.h"
#include "lang/processors/type_checker/constant_handler.h"
#include "lang/processors/type_checker/variable_handler.h"
#include "lang/processors/type_checker/expr_handler.h"

namespace lang {
namespace type_checker {

void StmtHandler::ProcessFuncBody(ast::BlockStmt *body,
                                  types::Tuple *func_results,
                                  types::TypeInfo *info,
                                  std::vector<issues::Issue>& issues) {
    StmtHandler handler(info, issues);
    Context ctx{
        .func_results = func_results,
        .labels = {},
        .can_break = false,
        .can_continue = false,
        .can_fallthrough = false,
    };
    handler.CheckBlockStmt(body, ctx);
}

void StmtHandler::CheckBlockStmt(ast::BlockStmt *block_stmt, Context ctx) {
    for (int i = 0; i < block_stmt->stmts_.size(); i++) {
        ast::Stmt *stmt = block_stmt->stmts_.at(i).get();
        ctx.is_last_stmt_in_block = (i == block_stmt->stmts_.size());
        CheckStmt(stmt, ctx);
    }
}

void StmtHandler::CheckStmt(ast::Stmt *stmt, Context ctx) {
    while (auto labeled_stmt = dynamic_cast<ast::LabeledStmt *>(stmt)) {
        stmt = labeled_stmt->stmt_.get();
        ctx.labels.insert({labeled_stmt->label_->name_, stmt});
    }
    
    if (auto block_stmt = dynamic_cast<ast::BlockStmt *>(stmt)) {
        ctx.can_fallthrough = false;
        CheckBlockStmt(block_stmt, ctx);
    } else if (auto decl_stmt = dynamic_cast<ast::DeclStmt *>(stmt)) {
        CheckDeclStmt(decl_stmt);
    } else if (auto assign_stmt = dynamic_cast<ast::AssignStmt *>(stmt)) {
        CheckAssignStmt(assign_stmt);
    } else if (auto expr_stmt = dynamic_cast<ast::ExprStmt *>(stmt)) {
        CheckExprStmt(expr_stmt);
    } else if (auto inc_dec_stmt = dynamic_cast<ast::IncDecStmt *>(stmt)) {
        CheckIncDecStmt(inc_dec_stmt);
    } else if (auto return_stmt = dynamic_cast<ast::ReturnStmt *>(stmt)) {
        CheckReturnStmt(return_stmt, ctx);
    } else if (auto if_stmt = dynamic_cast<ast::IfStmt *>(stmt)) {
        CheckIfStmt(if_stmt, ctx);
    } else if (auto switch_stmt = dynamic_cast<ast::SwitchStmt *>(stmt)) {
        if (!ast::IsTypeSwitchStmt(switch_stmt)) {
            CheckExprSwitchStmt(switch_stmt, ctx);
        } else {
            CheckTypeSwitchStmt(switch_stmt, ctx);
        }
    } else if (auto for_stmt = dynamic_cast<ast::ForStmt *>(stmt)) {
        CheckForStmt(for_stmt, ctx);
    } else if (auto branch_stmt = dynamic_cast<ast::BranchStmt *>(stmt)) {
        CheckBranchStmt(branch_stmt, ctx);
    } else {
        throw "internal error: unexpected stmt type";
    }
}

void StmtHandler::CheckDeclStmt(ast::DeclStmt *stmt) {
    switch (stmt->decl_->tok_) {
        case tokens::kType:
            for (auto& spec : stmt->decl_->specs_) {
                ast::TypeSpec *type_spec = static_cast<ast::TypeSpec *>(spec.get());
                types::TypeName *type_name =
                    static_cast<types::TypeName *>(info_->definitions().at(type_spec->name_.get()));
                
                TypeHandler::ProcessTypeName(type_name,
                                             type_spec,
                                             info_,
                                             issues_);
            }
            return;
        case tokens::kConst:{
            int64_t iota = 0;
            for (auto& spec : stmt->decl_->specs_) {
                ast::ValueSpec *value_spec = static_cast<ast::ValueSpec *>(spec.get());
                ast::Expr *type_expr = value_spec->type_.get();
                types::Type *type = nullptr;
                if (type_expr != nullptr) {
                    if (!TypeHandler::ProcessTypeExpr(type_expr, info_, issues_)) {
                        return;
                    }
                    type = info_->TypeOf(type_expr);
                }
                for (size_t i = 0; i < value_spec->names_.size(); i++) {
                    ast::Ident *name = value_spec->names_.at(i).get();
                    types::Constant *constant =
                        static_cast<types::Constant *>(info_->definitions().at(name));
                    
                    ast::Expr *value = nullptr;
                    if (value_spec->values_.size() > i) {
                        value = value_spec->values_.at(i).get();
                    }
                    ConstantHandler::ProcessConstant(constant,
                                                     type,
                                                     value,
                                                     iota,
                                                     info_,
                                                     issues_);
                }
                iota++;
            }
            return;
        }
        case tokens::kVar:
            for (auto& spec : stmt->decl_->specs_) {
                ast::ValueSpec *value_spec = static_cast<ast::ValueSpec *>(spec.get());
                ast::Expr *type_expr = value_spec->type_.get();
                types::Type *type = nullptr;
                if (type_expr != nullptr) {
                    if (!TypeHandler::ProcessTypeExpr(type_expr, info_, issues_)) {
                        return;
                    }
                    type = info_->TypeOf(type_expr);
                }
                if (value_spec->names_.size() > 1 && value_spec->names_.size() == 1) {
                    std::vector<types::Variable *> variables;
                    for (auto& name : value_spec->names_) {
                        types::Variable *variable =
                            static_cast<types::Variable *>(info_->definitions().at(name.get()));
                        variables.push_back(variable);
                    }
                    ast::Expr *value = value_spec->values_.at(0).get();
                    VariableHandler::ProcessVariables(variables,
                                                      type,
                                                      value,
                                                      info_,
                                                      issues_);
                } else {
                    for (size_t i = 0; i < value_spec->names_.size(); i++) {
                        ast::Ident *name = value_spec->names_.at(i).get();
                        types::Variable *variable =
                            static_cast<types::Variable *>(info_->definitions().at(name));
                        
                        ast::Expr *value = nullptr;
                        if (value_spec->values_.size() > i) {
                            value = value_spec->values_.at(i).get();
                        }
                        VariableHandler::ProcessVariable(variable,
                                                         type,
                                                         value,
                                                         info_,
                                                         issues_);
                    }
                }
            }
            break;
        default:
            throw "internal error: unexpected lang::ast::GenDecl";
    }
}

void StmtHandler::CheckAssignStmt(ast::AssignStmt *assign_stmt) {
    std::vector<types::Type *> lhs_types;
    std::vector<types::Type *> rhs_types;
    for (auto& lhs_expr : assign_stmt->lhs_) {
        ast::Ident *ident = dynamic_cast<ast::Ident *>(lhs_expr.get());
        types::Variable *var = dynamic_cast<types::Variable *>(info_->DefinitionOf(ident));
        if (assign_stmt->tok_ == tokens::kDefine && var != nullptr) {
            lhs_types.push_back(nullptr);
            continue;
        }
        if (!ExprHandler::ProcessExpr(lhs_expr.get(), info_, issues_)) {
            lhs_types.push_back(nullptr);
            continue;
        }
        types::ExprKind lhs_kind = info_->ExprKindOf(lhs_expr.get()).value();
        if (lhs_kind != types::ExprKind::kVariable) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            lhs_expr->start(),
                                            "invalid operation: expected addessable operand"));
            lhs_types.push_back(nullptr);
            continue;
        }
        types::Type *lhs_type = info_->TypeOf(lhs_expr.get());
        lhs_types.push_back(lhs_type);
    }
    for (auto& rhs_expr : assign_stmt->rhs_) {
        if (!ExprHandler::ProcessExpr(rhs_expr.get(), info_, issues_)) {
            rhs_types.push_back(nullptr);
            continue;
        }
        types::Type *rhs_type = info_->TypeOf(rhs_expr.get());
        rhs_types.push_back(rhs_type);
    }
    
    if (rhs_types.size() == 1 && rhs_types.at(0) != nullptr) {
        if (auto tuple = dynamic_cast<types::Tuple *>(rhs_types.at(0))) {
            rhs_types.clear();
            rhs_types.reserve(tuple->variables().size());
            for (auto var : tuple->variables()) {
                rhs_types.push_back(var->type());
            }
        }
    }
    if (rhs_types.size() == 1 && rhs_types.at(0) != nullptr) {
        types::ExprKind rhs_kind = info_->ExprKindOf(assign_stmt->rhs_.at(0).get()).value();
        if (rhs_kind == types::ExprKind::kValueOk) {
            if (lhs_types.size() > 2) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                assign_stmt->start(),
                                                "invalid operation: expected at most two operands "
                                                "to be assigned"));
                return;
            } else if (lhs_types.size() == 2) {
                rhs_types.push_back(info_->basic_types_.at(types::Basic::Kind::kUntypedBool));
            }
        }
    }
    
    if (lhs_types.size() != rhs_types.size()) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        assign_stmt->start(),
                                        "invalid operation: can not assign "
                                        + std::to_string(rhs_types.size()) + " values to "
                                        + std::to_string(lhs_types.size()) + " operands"));
    }
    for (int i = 0; i < lhs_types.size() && i < rhs_types.size(); i++) {
        types::Type *lhs_type = lhs_types.at(i);
        types::Type *rhs_type = rhs_types.at(i);
        
        if (assign_stmt->tok_ == tokens::kDefine) {
            ast::Ident *ident = dynamic_cast<ast::Ident *>(assign_stmt->lhs_.at(i).get());
            types::Variable *var = dynamic_cast<types::Variable *>(info_->DefinitionOf(ident));
            if (var != nullptr && rhs_type != nullptr) {
                var->type_ = rhs_type;
            }
            continue;
        }
        
        if (lhs_type == nullptr || rhs_type == nullptr) {
            continue;
        } else if (!types::IsAssignableTo(rhs_type, lhs_type)) {
            if (assign_stmt->rhs_.size() == assign_stmt->lhs_.size()) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                {assign_stmt->lhs_.at(i)->start(),
                                                 assign_stmt->rhs_.at(i)->start()},
                                                "can not assign value of type "
                                                + rhs_type->ToString() + "to operand of type "
                                                + lhs_type->ToString() + ""));
            } else {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                {assign_stmt->lhs_.at(i)->start(),
                                                 assign_stmt->rhs_.at(0)->start()},
                                                "can not assign argument to parameter"));
            }
        }
    }
}

void StmtHandler::CheckExprStmt(ast::ExprStmt *expr_stmt) {
    ExprHandler::ProcessExpr(expr_stmt->x_.get(), info_, issues_);
}

void StmtHandler::CheckIncDecStmt(ast::IncDecStmt *inc_dec_stmt) {
    if (!ExprHandler::ProcessExpr(inc_dec_stmt->x_.get(), info_, issues_)) {
        return;
    }
    
    types::Type *x_type = info_->TypeOf(inc_dec_stmt->x_.get());
    types::Basic *underlying_type = dynamic_cast<types::Basic *>(x_type->Underlying());
    if (underlying_type == nullptr ||
        !(underlying_type->info() & types::Basic::Info::kIsInteger)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        inc_dec_stmt->start(),
                                        "invalid operation: expected integer type"));
    }
}

void StmtHandler::CheckReturnStmt(ast::ReturnStmt *return_stmt, Context ctx) {
    std::vector<types::Type *> result_types;
    bool ok = true;
    for (auto& result_expr : return_stmt->results_) {
        ok = ExprHandler::ProcessExpr(result_expr.get(), info_, issues_) && ok;
        types::Type *result_type = info_->TypeOf(result_expr.get());
        result_types.push_back(result_type);
    }
    if (!ok) {
        return;
    }
    
    if (return_stmt->results_.size() == 1) {
        types::Tuple *result_tuple = dynamic_cast<types::Tuple *>(result_types.at(0));
        if (result_tuple) {
            if (!types::IsAssignableTo(result_tuple, ctx.func_results)) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                return_stmt->start(),
                                                "invalid operation: results can not be assigned to "
                                                "function result types"));
            }
            return;
        }
    }
    if (result_types.size() != ctx.func_results->variables().size()) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        return_stmt->start(),
                                        "invalid operation: number of results does not match"
                                        "expected number of results"));
        return;
    }
    for (int i = 0; i < result_types.size(); i++) {
        types::Type *expected_result_type = ctx.func_results->variables().at(i)->type();
        types::Type *given_result_type = result_types.at(i);
        ast::Expr *result_expr = return_stmt->results_.at(i).get();
        if (!types::IsAssignableTo(given_result_type, expected_result_type)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            result_expr->start(),
                                            "invalid operation: result can not be assigned to "
                                            "function result type"));
        }
    }
    return;
}

void StmtHandler::CheckIfStmt(ast::IfStmt *if_stmt, Context ctx) {
    if (if_stmt->init_) {
        CheckStmt(if_stmt->init_.get(), ctx);
    }
    if (ExprHandler::ProcessExpr(if_stmt->cond_.get(), info_, issues_)) {
        types::Type *cond_type = info_->TypeOf(if_stmt->cond_.get());
        types::Basic *underlying_type = dynamic_cast<types::Basic *>(cond_type->Underlying());
        if (underlying_type == nullptr ||
            !(underlying_type->info() & types::Basic::Info::kIsBoolean)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            if_stmt->cond_->start(),
                                            "invalid operation: expected boolean type"));
        }
    }
    
    ctx.can_fallthrough = false;
    CheckBlockStmt(if_stmt->body_.get(), ctx);
    if (if_stmt->else_) {
        CheckStmt(if_stmt->else_.get(), ctx);
    }
}

void StmtHandler::CheckExprSwitchStmt(ast::SwitchStmt *switch_stmt, Context ctx) {
    if (switch_stmt->init_) {
        CheckStmt(switch_stmt->init_.get(), ctx);
    }
    types::Type *tag_type = info_->basic_types_.at(types::Basic::Kind::kUntypedBool);
    if (switch_stmt->tag_) {
        if (ExprHandler::ProcessExpr(switch_stmt->tag_.get(), info_, issues_)) {
            tag_type = info_->TypeOf(switch_stmt->tag_.get());
        } else {
            tag_type = nullptr;
        }
    }
    ctx.can_break = true;
    bool seen_default = false;
    for (int i = 0; i < switch_stmt->body_->stmts_.size(); i++) {
        std::unique_ptr<ast::Stmt>& stmt = switch_stmt->body_->stmts_.at(i);
        ast::CaseClause *case_clause = static_cast<ast::CaseClause *>(stmt.get());
        if (case_clause->tok_ == tokens::kDefault) {
            if (seen_default) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                case_clause->start(),
                                                "duplicate default case in switch statement"));
            } else {
                seen_default = true;
            }
        }
        ctx.can_fallthrough = (i < switch_stmt->body_->stmts_.size() - 1);
        CheckExprCaseClause(case_clause, tag_type, ctx);
    }
}

void StmtHandler::CheckExprCaseClause(ast::CaseClause *case_clause,
                                      types::Type *tag_type,
                                      Context ctx) {
    for (auto& expr : case_clause->cond_vals_) {
        if (ExprHandler::ProcessExpr(expr.get(), info_, issues_) &&
            tag_type != nullptr) {
            types::Type *expr_type = info_->TypeOf(expr.get());
            if (!types::IsComparable(tag_type, expr_type, tokens::kEql)) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "invalid operation: can not compare value "
                                                "expression with switch tag"));
            }
        }
    }
    for (int i = 0; i < case_clause->body_.size(); i++) {
        ast::Stmt *stmt = case_clause->body_.at(i).get();
        ctx.is_last_stmt_in_block = (i == case_clause->body_.size());
        CheckStmt(stmt, ctx);
    }
}

void StmtHandler::CheckTypeSwitchStmt(ast::SwitchStmt *switch_stmt, Context ctx) {
    ast::TypeAssertExpr *type_assert_expr = nullptr;
    if (switch_stmt->init_) {
        ast::AssignStmt *assign_stmt = static_cast<ast::AssignStmt *>(switch_stmt->init_.get());
        type_assert_expr = static_cast<ast::TypeAssertExpr *>(assign_stmt->rhs_.at(0).get());
    } else {
        type_assert_expr = static_cast<ast::TypeAssertExpr *>(switch_stmt->tag_.get());
    }
    ast::Expr *x_expr = type_assert_expr->x_.get();
    types::Type *x_type = nullptr;
    if (ExprHandler::ProcessExpr(x_expr, info_, issues_)) {
        x_type = info_->TypeOf(x_expr);
    }
    ctx.can_break = true;
    ctx.can_fallthrough = false;
    bool seen_default = false;
    for (int i = 0; i < switch_stmt->body_->stmts_.size(); i++) {
        std::unique_ptr<ast::Stmt>& stmt = switch_stmt->body_->stmts_.at(i);
        ast::CaseClause *case_clause = static_cast<ast::CaseClause *>(stmt.get());
        if (case_clause->tok_ == tokens::kDefault) {
            if (seen_default) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                case_clause->start(),
                                                "duplicate default case in switch statement"));
            } else {
                seen_default = true;
            }
        }
        CheckTypeCaseClause(case_clause, x_type, ctx);
    }
}

void StmtHandler::CheckTypeCaseClause(ast::CaseClause *case_clause,
                                      types::Type *x_type,
                                      Context ctx) {
    types::Type *implicit_x_type = x_type;
    for (auto& expr : case_clause->cond_vals_) {
        if (TypeHandler::ProcessTypeExpr(expr.get(), info_, issues_) &&
            x_type != nullptr) {
            types::Type *specialised_type = info_->TypeOf(expr.get());
            if (!types::IsAssertableTo(x_type, specialised_type)) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "invalid operation: value of type switch tag can "
                                                "never have the given type"));
            } else if (case_clause->cond_vals_.size() == 1) {
                implicit_x_type = specialised_type;
            }
        }
    }
    types::Variable *implicit_x = static_cast<types::Variable *>(info_->ImplicitOf(case_clause));
    implicit_x->type_ = implicit_x_type;
    
    for (int i = 0; i < case_clause->body_.size(); i++) {
        ast::Stmt *stmt = case_clause->body_.at(i).get();
        ctx.is_last_stmt_in_block = (i == case_clause->body_.size());
        CheckStmt(stmt, ctx);
    }
}

void StmtHandler::CheckForStmt(ast::ForStmt *for_stmt, Context ctx) {
    if (for_stmt->init_) {
        CheckStmt(for_stmt->init_.get(), ctx);
    }
    if (ExprHandler::ProcessExpr(for_stmt->cond_.get(), info_, issues_)) {
        types::Type *cond_type = info_->TypeOf(for_stmt->cond_.get());
        types::Basic *underlying_type = dynamic_cast<types::Basic *>(cond_type->Underlying());
        if (underlying_type == nullptr ||
            !(underlying_type->info() & types::Basic::Info::kIsBoolean)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            for_stmt->cond_->start(),
                                            "invalid operation: expected boolean type"));
        }
    }
    if (for_stmt->post_) {
        CheckStmt(for_stmt->post_.get(), ctx);
    }
    
    ctx.can_break = true;
    ctx.can_continue = true;
    ctx.can_fallthrough = false;
    CheckBlockStmt(for_stmt->body_.get(), ctx);
}

void StmtHandler::CheckBranchStmt(ast::BranchStmt *branch_stmt, Context ctx) {
    if (!ctx.is_last_stmt_in_block) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        branch_stmt->start(),
                                        "branch statement is not last in block"));
        return;
    }
    ast::Stmt *labeled_destination = nullptr;
    bool destination_is_labeled_loop = false;
    bool destination_is_labeled_switch = false;
    if (branch_stmt->label_) {
        if(branch_stmt->tok_ == tokens::kFallthrough) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            branch_stmt->start(),
                                            "fallthrough with label is now allowed"));
            return;
        }
        auto it = ctx.labels.find(branch_stmt->label_->name_);
        if (it == ctx.labels.end()) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            branch_stmt->start(),
                                            "branch label does not refer to any enclosing statement"));
            return;
        }
        labeled_destination = it->second;
        destination_is_labeled_loop =
            (dynamic_cast<ast::ForStmt *>(labeled_destination) != nullptr);
        destination_is_labeled_switch =
            (dynamic_cast<ast::SwitchStmt *>(labeled_destination) != nullptr);
    }
    
    switch (branch_stmt->tok_) {
        case tokens::kBreak:
            if (labeled_destination == nullptr) {
                if (!ctx.can_break) {
                    issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                    issues::Severity::Error,
                                                    branch_stmt->start(),
                                                    "can not break: no enclosing switch or for "
                                                    "statement"));
                }
            } else if (!destination_is_labeled_loop && !destination_is_labeled_switch) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                branch_stmt->start(),
                                                "break label does not refer to an enclosing switch "
                                                " or for statement"));
            }
            return;
        case tokens::kContinue:
            if (labeled_destination == nullptr) {
                if (!ctx.can_continue) {
                    issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                    issues::Severity::Error,
                                                    branch_stmt->start(),
                                                    "can not continue: no enclosing for "
                                                    "statement"));
                }
            } else if (!destination_is_labeled_loop) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                branch_stmt->start(),
                                                "continue label does not refer to an enclosing for "
                                                "statement"));
            }
            return;
        case tokens::kFallthrough:
            if (!ctx.can_fallthrough) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                branch_stmt->start(),
                                                "can not fallthrough: no expression type switch "
                                                "case immedately after"));
            }
            return;
        default:
            throw "internal error: unexpected branch statement";
    }
}

}
}

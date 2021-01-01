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
                                  types::InfoBuilder& info_builder,
                                  std::vector<issues::Issue>& issues) {
    StmtHandler handler(info_builder, issues);
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
    for (int i = 0; i < block_stmt->stmts().size(); i++) {
        ast::Stmt *stmt = block_stmt->stmts().at(i);
        ctx.is_last_stmt_in_block = (i == block_stmt->stmts().size());
        CheckStmt(stmt, ctx);
    }
}

void StmtHandler::CheckStmt(ast::Stmt *stmt, Context ctx) {
    while (stmt->node_kind() == ast::NodeKind::kLabeledStmt) {
        stmt = static_cast<ast::LabeledStmt *>(stmt)->stmt();
        ctx.labels.insert({static_cast<ast::LabeledStmt *>(stmt)->label()->name(), stmt});
    }
    switch (stmt->node_kind()) {
        case ast::NodeKind::kBlockStmt:
            ctx.can_fallthrough = false;
            CheckBlockStmt(static_cast<ast::BlockStmt *>(stmt), ctx);
            break;
        case ast::NodeKind::kDeclStmt:
            CheckDeclStmt(static_cast<ast::DeclStmt *>(stmt));
            break;
        case ast::NodeKind::kAssignStmt:
            CheckAssignStmt(static_cast<ast::AssignStmt *>(stmt));
            break;
        case ast::NodeKind::kExprStmt:
            CheckExprStmt(static_cast<ast::ExprStmt *>(stmt));
            break;
        case ast::NodeKind::kIncDecStmt:
            CheckIncDecStmt(static_cast<ast::IncDecStmt *>(stmt));
            break;
        case ast::NodeKind::kReturnStmt:
            CheckReturnStmt(static_cast<ast::ReturnStmt *>(stmt), ctx);
            break;
        case ast::NodeKind::kIfStmt:
            CheckIfStmt(static_cast<ast::IfStmt *>(stmt), ctx);
            break;
        case ast::NodeKind::kExprSwitchStmt:
            CheckExprSwitchStmt(static_cast<ast::ExprSwitchStmt *>(stmt), ctx);
            break;
        case ast::NodeKind::kTypeSwitchStmt:
            CheckTypeSwitchStmt(static_cast<ast::TypeSwitchStmt *>(stmt), ctx);
            break;
        case ast::NodeKind::kForStmt:
            CheckForStmt(static_cast<ast::ForStmt *>(stmt), ctx);
            break;
        case ast::NodeKind::kBranchStmt:
            CheckBranchStmt(static_cast<ast::BranchStmt *>(stmt), ctx);
            break;
        default:
            throw "internal error: unexpected stmt type";
    }
}

void StmtHandler::CheckDeclStmt(ast::DeclStmt *stmt) {
    switch (stmt->decl()->tok()) {
        case tokens::kType:
            for (ast::Spec *spec : stmt->decl()->specs()) {
                ast::TypeSpec *type_spec = static_cast<ast::TypeSpec *>(spec);
                types::TypeName *type_name =
                    static_cast<types::TypeName *>(info_->definitions().at(type_spec->name()));
                
                TypeHandler::ProcessTypeParametersOfTypeName(type_name,
                                                             type_spec,
                                                             info_builder_,
                                                             issues_);
                TypeHandler::ProcessUnderlyingTypeOfTypeName(type_name,
                                                             type_spec,
                                                             info_builder_,
                                                             issues_);
            }
            return;
        case tokens::kConst:{
            int64_t iota = 0;
            for (ast::Spec *spec : stmt->decl()->specs()) {
                ast::ValueSpec *value_spec = static_cast<ast::ValueSpec *>(spec);
                ast::Expr *type_expr = value_spec->type();
                types::Type *type = nullptr;
                if (type_expr != nullptr) {
                    if (!TypeHandler::ProcessTypeExpr(type_expr, info_builder_, issues_)) {
                        return;
                    }
                    type = info_->TypeOf(type_expr);
                }
                for (size_t i = 0; i < value_spec->names().size(); i++) {
                    ast::Ident *name = value_spec->names().at(i);
                    types::Constant *constant =
                        static_cast<types::Constant *>(info_->definitions().at(name));
                    
                    ast::Expr *value = nullptr;
                    if (value_spec->values().size() > i) {
                        value = value_spec->values().at(i);
                    }
                    ConstantHandler::ProcessConstant(constant,
                                                     type,
                                                     value,
                                                     iota,
                                                     info_builder_,
                                                     issues_);
                }
                iota++;
            }
            return;
        }
        case tokens::kVar:
            for (ast::Spec *spec : stmt->decl()->specs()) {
                ast::ValueSpec *value_spec = static_cast<ast::ValueSpec *>(spec);
                ast::Expr *type_expr = value_spec->type();
                types::Type *type = nullptr;
                if (type_expr != nullptr) {
                    if (!TypeHandler::ProcessTypeExpr(type_expr, info_builder_, issues_)) {
                        return;
                    }
                    type = info_->TypeOf(type_expr);
                }
                if (value_spec->names().size() > 1 && value_spec->names().size() == 1) {
                    std::vector<types::Variable *> variables;
                    for (ast::Ident *name : value_spec->names()) {
                        types::Variable *variable =
                            static_cast<types::Variable *>(info_->definitions().at(name));
                        variables.push_back(variable);
                    }
                    ast::Expr *value = value_spec->values().at(0);
                    VariableHandler::ProcessVariables(variables,
                                                      type,
                                                      value,
                                                      info_builder_,
                                                      issues_);
                } else {
                    for (size_t i = 0; i < value_spec->names().size(); i++) {
                        ast::Ident *name = value_spec->names().at(i);
                        types::Variable *variable =
                            static_cast<types::Variable *>(info_->definitions().at(name));
                        
                        ast::Expr *value = nullptr;
                        if (value_spec->values().size() > i) {
                            value = value_spec->values().at(i);
                        }
                        VariableHandler::ProcessVariable(variable,
                                                         type,
                                                         value,
                                                         info_builder_,
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
    for (ast::Expr *lhs_expr : assign_stmt->lhs()) {
        if (lhs_expr->node_kind() != ast::NodeKind::kIdent) {
            lhs_types.push_back(nullptr);
            continue;
        }
        ast::Ident *ident = static_cast<ast::Ident *>(lhs_expr);
        types::Object *obj = info_->DefinitionOf(ident);
        if (obj == nullptr ||
            obj->object_kind() != types::ObjectKind::kVariable) {
            lhs_types.push_back(nullptr);
            continue;
        }
        if (assign_stmt->tok() == tokens::kDefine) {
            lhs_types.push_back(nullptr);
            continue;
        }
        if (!ExprHandler::ProcessExpr(lhs_expr, info_builder_, issues_)) {
            lhs_types.push_back(nullptr);
            continue;
        }
        types::ExprKind lhs_kind = info_->ExprKindOf(lhs_expr).value();
        if (lhs_kind != types::ExprKind::kVariable) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            lhs_expr->start(),
                                            "invalid operation: expected addessable operand"));
            lhs_types.push_back(nullptr);
            continue;
        }
        types::Type *lhs_type = info_->TypeOf(lhs_expr);
        lhs_types.push_back(lhs_type);
    }
    for (ast::Expr *rhs_expr : assign_stmt->rhs()) {
        if (!ExprHandler::ProcessExpr(rhs_expr, info_builder_, issues_)) {
            rhs_types.push_back(nullptr);
            continue;
        }
        types::Type *rhs_type = info_->TypeOf(rhs_expr);
        rhs_types.push_back(rhs_type);
    }
    
    if (rhs_types.size() == 1 &&
        rhs_types.at(0) != nullptr &&
        rhs_types.at(0)->type_kind() == types::TypeKind::kTuple) {
        types::Tuple *tuple = static_cast<types::Tuple *>(rhs_types.at(0));
        rhs_types.clear();
        rhs_types.reserve(tuple->variables().size());
        for (types::Variable *var : tuple->variables()) {
            rhs_types.push_back(var->type());
        }
    }
    if (rhs_types.size() == 1 && rhs_types.at(0) != nullptr) {
        types::ExprKind rhs_kind = info_->ExprKindOf(assign_stmt->rhs().at(0)).value();
        if (rhs_kind == types::ExprKind::kValueOk) {
            if (lhs_types.size() > 2) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                assign_stmt->start(),
                                                "invalid operation: expected at most two operands "
                                                "to be assigned"));
                return;
            } else if (lhs_types.size() == 2) {
                rhs_types.push_back(info_->basic_type(types::Basic::Kind::kUntypedBool));
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
        
        if (assign_stmt->tok() == tokens::kDefine &&
            assign_stmt->lhs().at(i)->node_kind() == ast::NodeKind::kIdent) {
            ast::Ident *ident = static_cast<ast::Ident *>(assign_stmt->lhs().at(i));
            types::Object *obj = info_->DefinitionOf(ident);
            if (obj->object_kind() == types::ObjectKind::kVariable && rhs_type != nullptr) {
                info_builder_.SetObjectType(static_cast<types::Variable *>(obj), rhs_type);
            }
            continue;
        }
        
        if (lhs_type == nullptr || rhs_type == nullptr) {
            continue;
        } else if (!types::IsAssignableTo(rhs_type, lhs_type)) {
            if (assign_stmt->rhs().size() == assign_stmt->lhs().size()) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                {assign_stmt->lhs().at(i)->start(),
                                                 assign_stmt->rhs().at(i)->start()},
                                                "can not assign value of type "
                                                + rhs_type->ToString(types::StringRep::kShort)
                                                + "to operand of type "
                                                + lhs_type->ToString(types::StringRep::kShort)));
            } else {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                {assign_stmt->lhs().at(i)->start(),
                                                 assign_stmt->rhs().at(0)->start()},
                                                "can not assign argument to parameter"));
            }
        }
    }
}

void StmtHandler::CheckExprStmt(ast::ExprStmt *expr_stmt) {
    ExprHandler::ProcessExpr(expr_stmt->x(), info_builder_, issues_);
}

void StmtHandler::CheckIncDecStmt(ast::IncDecStmt *inc_dec_stmt) {
    if (!ExprHandler::ProcessExpr(inc_dec_stmt->x(), info_builder_, issues_)) {
        return;
    }
    
    types::Type *x_type = info_->TypeOf(inc_dec_stmt->x());
    types::Type *x_underlying = types::UnderlyingOf(x_type);
    if (x_underlying == nullptr ||
        x_underlying->type_kind() != types::TypeKind::kBasic ||
        !(static_cast<types::Basic *>(x_underlying)->info() & types::Basic::Info::kIsInteger)) {
        issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                        issues::Severity::Error,
                                        inc_dec_stmt->start(),
                                        "invalid operation: expected integer type"));
    }
}

void StmtHandler::CheckReturnStmt(ast::ReturnStmt *return_stmt, Context ctx) {
    std::vector<types::Type *> result_types;
    bool ok = true;
    for (ast::Expr *result_expr : return_stmt->results()) {
        ok = ExprHandler::ProcessExpr(result_expr, info_builder_, issues_) && ok;
        types::Type *result_type = info_->TypeOf(result_expr);
        result_types.push_back(result_type);
    }
    if (!ok) {
        return;
    }
    
    if (return_stmt->results().size() == 1 &&
        result_types.at(0)->type_kind() == types::TypeKind::kTuple) {
        types::Tuple *result_tuple = static_cast<types::Tuple *>(result_types.at(0));
        if (!types::IsAssignableTo(result_tuple, ctx.func_results)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            return_stmt->start(),
                                            "invalid operation: results can not be assigned to "
                                            "function result types"));
        }
        return;
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
        ast::Expr *result_expr = return_stmt->results().at(i);
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
    if (if_stmt->init_stmt()) {
        CheckStmt(if_stmt->init_stmt(), ctx);
    }
    if (ExprHandler::ProcessExpr(if_stmt->cond_expr(), info_builder_, issues_)) {
        types::Type *cond_type = info_->TypeOf(if_stmt->cond_expr());
        types::Type *underlying = types::UnderlyingOf(cond_type);
        if (underlying == nullptr ||
            underlying->type_kind() != types::TypeKind::kBasic ||
            !(static_cast<types::Basic *>(underlying)->info() & types::Basic::Info::kIsBoolean)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            if_stmt->cond_expr()->start(),
                                            "invalid operation: expected boolean type"));
        }
    }
    
    ctx.can_fallthrough = false;
    CheckBlockStmt(if_stmt->body(), ctx);
    if (if_stmt->else_stmt()) {
        CheckStmt(if_stmt->else_stmt(), ctx);
    }
}

void StmtHandler::CheckExprSwitchStmt(ast::ExprSwitchStmt *switch_stmt, Context ctx) {
    if (switch_stmt->init_stmt()) {
        CheckStmt(switch_stmt->init_stmt(), ctx);
    }
    types::Type *tag_type = info_->basic_type(types::Basic::Kind::kUntypedBool);
    if (switch_stmt->tag_expr()) {
        if (ExprHandler::ProcessExpr(switch_stmt->tag_expr(), info_builder_, issues_)) {
            tag_type = info_->TypeOf(switch_stmt->tag_expr());
        } else {
            tag_type = nullptr;
        }
    }
    ctx.can_break = true;
    bool seen_default = false;
    for (int i = 0; i < switch_stmt->body()->stmts().size(); i++) {
        ast::Stmt *stmt = switch_stmt->body()->stmts().at(i);
        ast::CaseClause *case_clause = static_cast<ast::CaseClause *>(stmt);
        if (case_clause->tok() == tokens::kDefault) {
            if (seen_default) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                case_clause->start(),
                                                "duplicate default case in switch statement"));
            } else {
                seen_default = true;
            }
        }
        ctx.can_fallthrough = (i < switch_stmt->body()->stmts().size() - 1);
        CheckExprCaseClause(case_clause, tag_type, ctx);
    }
}

void StmtHandler::CheckExprCaseClause(ast::CaseClause *case_clause,
                                      types::Type *tag_type,
                                      Context ctx) {
    for (ast::Expr *expr : case_clause->cond_vals()) {
        if (ExprHandler::ProcessExpr(expr, info_builder_, issues_) &&
            tag_type != nullptr) {
            types::Type *expr_type = info_->TypeOf(expr);
            if (!types::IsComparable(tag_type, expr_type)) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "invalid operation: can not compare value "
                                                "expression with switch tag"));
            }
        }
    }
    for (int i = 0; i < case_clause->body().size(); i++) {
        ast::Stmt *stmt = case_clause->body().at(i);
        ctx.is_last_stmt_in_block = (i == case_clause->body().size());
        CheckStmt(stmt, ctx);
    }
}

void StmtHandler::CheckTypeSwitchStmt(ast::TypeSwitchStmt *switch_stmt, Context ctx) {
    types::Type *tag_type = nullptr;
    if (ExprHandler::ProcessExpr(switch_stmt->tag_expr(), info_builder_, issues_)) {
        tag_type = info_->TypeOf(switch_stmt->tag_expr());
    }
    ctx.can_break = true;
    ctx.can_fallthrough = false;
    bool seen_default = false;
    for (int i = 0; i < switch_stmt->body()->stmts().size(); i++) {
        ast::Stmt *stmt = switch_stmt->body()->stmts().at(i);
        ast::CaseClause *case_clause = static_cast<ast::CaseClause *>(stmt);
        if (case_clause->tok() == tokens::kDefault) {
            if (seen_default) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                case_clause->start(),
                                                "duplicate default case in switch statement"));
            } else {
                seen_default = true;
            }
        }
        CheckTypeCaseClause(case_clause, tag_type, ctx);
    }
}

void StmtHandler::CheckTypeCaseClause(ast::CaseClause *case_clause,
                                      types::Type *tag_type,
                                      Context ctx) {
    types::Type *implicit_tag_type = tag_type;
    for (ast::Expr *expr : case_clause->cond_vals()) {
        if (TypeHandler::ProcessTypeExpr(expr, info_builder_, issues_) && tag_type != nullptr) {
            types::Type *specialised_type = info_->TypeOf(expr);
            if (!types::IsAssertableTo(tag_type, specialised_type)) {
                issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                                issues::Severity::Error,
                                                expr->start(),
                                                "invalid operation: value of type switch tag can "
                                                "never have the given type"));
            } else if (case_clause->cond_vals().size() == 1) {
                implicit_tag_type = specialised_type;
            }
        }
    }
    types::Variable *implicit_tag = static_cast<types::Variable *>(info_->ImplicitOf(case_clause));
    info_builder_.SetObjectType(implicit_tag, implicit_tag_type);
    
    for (int i = 0; i < case_clause->body().size(); i++) {
        ast::Stmt *stmt = case_clause->body().at(i);
        ctx.is_last_stmt_in_block = (i == case_clause->body().size());
        CheckStmt(stmt, ctx);
    }
}

void StmtHandler::CheckForStmt(ast::ForStmt *for_stmt, Context ctx) {
    if (for_stmt->init_stmt() != nullptr) {
        CheckStmt(for_stmt->init_stmt(), ctx);
    }
    if (ExprHandler::ProcessExpr(for_stmt->cond_expr(), info_builder_, issues_)) {
        types::Type *cond_type = info_->TypeOf(for_stmt->cond_expr());
        types::Type *underlying = types::UnderlyingOf(cond_type);
        if (underlying == nullptr ||
            underlying->type_kind() != types::TypeKind::kBasic ||
            !(static_cast<types::Basic *>(underlying)->info() & types::Basic::Info::kIsBoolean)) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            for_stmt->cond_expr()->start(),
                                            "invalid operation: expected boolean type"));
        }
    }
    if (for_stmt->post_stmt()) {
        CheckStmt(for_stmt->post_stmt(), ctx);
    }
    
    ctx.can_break = true;
    ctx.can_continue = true;
    ctx.can_fallthrough = false;
    CheckBlockStmt(for_stmt->body(), ctx);
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
    if (branch_stmt->label() != nullptr) {
        if(branch_stmt->tok() == tokens::kFallthrough) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            branch_stmt->start(),
                                            "fallthrough with label is now allowed"));
            return;
        }
        auto it = ctx.labels.find(branch_stmt->label()->name());
        if (it == ctx.labels.end()) {
            issues_.push_back(issues::Issue(issues::Origin::TypeChecker,
                                            issues::Severity::Error,
                                            branch_stmt->start(),
                                            "branch label does not refer to any enclosing statement"));
            return;
        }
        labeled_destination = it->second;
        destination_is_labeled_loop = (labeled_destination->node_kind() == ast::NodeKind::kForStmt);
        destination_is_labeled_switch =
            (labeled_destination->node_kind() == ast::NodeKind::kExprSwitchStmt) ||
            (labeled_destination->node_kind() == ast::NodeKind::kTypeSwitchStmt);
    }
    
    switch (branch_stmt->tok()) {
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

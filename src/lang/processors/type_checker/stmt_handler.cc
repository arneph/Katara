//
//  stmt_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 11/22/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "stmt_handler.h"

#include "lang/processors/type_checker/type_resolver.h"
#include "lang/representation/ast/ast_util.h"
#include "lang/representation/types/types_util.h"

namespace lang {
namespace type_checker {

void StmtHandler::CheckFuncBody(ast::BlockStmt* body, types::Tuple* func_results) {
  Context ctx{
      .func_results = func_results,
      .labels = {},
      .can_break = false,
      .can_continue = false,
      .can_fallthrough = false,
  };
  CheckBlockStmt(body, ctx);
}

void StmtHandler::CheckBlockStmt(ast::BlockStmt* block_stmt, Context ctx) {
  for (size_t i = 0; i < block_stmt->stmts().size(); i++) {
    ast::Stmt* stmt = block_stmt->stmts().at(i);
    ctx.is_last_stmt_in_block = (i == block_stmt->stmts().size());
    CheckStmt(stmt, ctx);
  }
}

void StmtHandler::CheckStmt(ast::Stmt* stmt, Context ctx) {
  while (stmt->node_kind() == ast::NodeKind::kLabeledStmt) {
    stmt = static_cast<ast::LabeledStmt*>(stmt)->stmt();
    ctx.labels.insert({static_cast<ast::LabeledStmt*>(stmt)->label()->name(), stmt});
  }
  switch (stmt->node_kind()) {
    case ast::NodeKind::kBlockStmt:
      ctx.can_fallthrough = false;
      CheckBlockStmt(static_cast<ast::BlockStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kDeclStmt:
      CheckDeclStmt(static_cast<ast::DeclStmt*>(stmt));
      break;
    case ast::NodeKind::kAssignStmt:
      CheckAssignStmt(static_cast<ast::AssignStmt*>(stmt));
      break;
    case ast::NodeKind::kExprStmt:
      CheckExprStmt(static_cast<ast::ExprStmt*>(stmt));
      break;
    case ast::NodeKind::kIncDecStmt:
      CheckIncDecStmt(static_cast<ast::IncDecStmt*>(stmt));
      break;
    case ast::NodeKind::kReturnStmt:
      CheckReturnStmt(static_cast<ast::ReturnStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kIfStmt:
      CheckIfStmt(static_cast<ast::IfStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kExprSwitchStmt:
      CheckExprSwitchStmt(static_cast<ast::ExprSwitchStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kTypeSwitchStmt:
      CheckTypeSwitchStmt(static_cast<ast::TypeSwitchStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kForStmt:
      CheckForStmt(static_cast<ast::ForStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kBranchStmt:
      CheckBranchStmt(static_cast<ast::BranchStmt*>(stmt), ctx);
      break;
    default:
      throw "internal error: unexpected stmt type";
  }
}

void StmtHandler::CheckDeclStmt(ast::DeclStmt* stmt) {
  switch (stmt->decl()->tok()) {
    case tokens::kType:
      for (ast::Spec* spec : stmt->decl()->specs()) {
        ast::TypeSpec* type_spec = static_cast<ast::TypeSpec*>(spec);
        types::TypeName* type_name =
            static_cast<types::TypeName*>(info()->definitions().at(type_spec->name()));

        type_resolver().decl_handler().ProcessTypeName(type_name, type_spec);
      }
      return;
    case tokens::kConst: {
      int64_t iota = 0;
      for (ast::Spec* spec : stmt->decl()->specs()) {
        ast::ValueSpec* value_spec = static_cast<ast::ValueSpec*>(spec);
        ast::Expr* type_expr = value_spec->type();
        for (size_t i = 0; i < value_spec->names().size(); i++) {
          ast::Ident* name = value_spec->names().at(i);
          types::Constant* constant = static_cast<types::Constant*>(info()->definitions().at(name));

          ast::Expr* value = nullptr;
          if (value_spec->values().size() > i) {
            value = value_spec->values().at(i);
          }
          type_resolver().decl_handler().ProcessConstant(constant, type_expr, value, iota);
        }
        iota++;
      }
      return;
    }
    case tokens::kVar:
      for (ast::Spec* spec : stmt->decl()->specs()) {
        ast::ValueSpec* value_spec = static_cast<ast::ValueSpec*>(spec);
        ast::Expr* type_expr = value_spec->type();
        if (value_spec->names().size() > 1 && value_spec->names().size() == 1) {
          std::vector<types::Variable*> variables;
          for (ast::Ident* name : value_spec->names()) {
            types::Variable* variable =
                static_cast<types::Variable*>(info()->definitions().at(name));
            variables.push_back(variable);
          }
          ast::Expr* value = value_spec->values().at(0);
          type_resolver().decl_handler().ProcessVariables(variables, type_expr, value);
        } else {
          for (size_t i = 0; i < value_spec->names().size(); i++) {
            ast::Ident* name = value_spec->names().at(i);
            types::Variable* variable =
                static_cast<types::Variable*>(info()->definitions().at(name));

            ast::Expr* value = nullptr;
            if (value_spec->values().size() > i) {
              value = value_spec->values().at(i);
            }
            type_resolver().decl_handler().ProcessVariable(variable, type_expr, value);
          }
        }
      }
      break;
    default:
      throw "internal error: unexpected lang::ast::GenDecl";
  }
}

void StmtHandler::CheckAssignStmt(ast::AssignStmt* assign_stmt) {
  std::vector<types::Type*> lhs_types;
  std::vector<types::Type*> rhs_types;
  for (ast::Expr* lhs_expr : assign_stmt->lhs()) {
    bool is_defined_var;
    if (assign_stmt->tok() != tokens::kDefine || lhs_expr->node_kind() != ast::NodeKind::kIdent) {
      is_defined_var = false;
    } else {
      ast::Ident* ident = static_cast<ast::Ident*>(lhs_expr);
      types::Object* obj = info()->DefinitionOf(ident);
      is_defined_var = (obj != nullptr && obj->object_kind() == types::ObjectKind::kVariable);
    }
    if (is_defined_var) {
      lhs_types.push_back(nullptr);
      continue;
    }
    if (!type_resolver().expr_handler().CheckExpr(lhs_expr)) {
      lhs_types.push_back(nullptr);
      continue;
    }
    types::ExprInfo lhs_info = info()->ExprInfoOf(lhs_expr).value();
    if (!lhs_info.is_addressable()) {
      issues().Add(issues::kUnexpectedAssignStmtLhsExprKind, lhs_expr->start(),
                   "expression is not addressable");
      lhs_types.push_back(nullptr);
      continue;
    }
    lhs_types.push_back(lhs_info.type());
  }
  for (ast::Expr* rhs_expr : assign_stmt->rhs()) {
    rhs_types.push_back(type_resolver().expr_handler().CheckValueExpr(rhs_expr));
  }

  if (rhs_types.size() == 1 && rhs_types.at(0) != nullptr &&
      rhs_types.at(0)->type_kind() == types::TypeKind::kTuple) {
    types::Tuple* tuple = static_cast<types::Tuple*>(rhs_types.at(0));
    rhs_types.clear();
    rhs_types.reserve(tuple->variables().size());
    for (types::Variable* var : tuple->variables()) {
      rhs_types.push_back(var->type());
    }
  }
  if (rhs_types.size() == 1 && rhs_types.at(0) != nullptr) {
    types::ExprInfo rhs_info = info()->ExprInfoOf(assign_stmt->rhs().at(0)).value();
    if (rhs_info.kind() == types::ExprInfo::Kind::kValueOk) {
      if (lhs_types.size() > 2) {
        issues().Add(issues::kMismatchedAssignStmtOperandCountForValueOkRhs, assign_stmt->start(),
                     "invalid operation: expected at most two operands to be assigned");
        return;
      } else if (lhs_types.size() == 2) {
        rhs_types.push_back(info()->basic_type(types::Basic::Kind::kUntypedBool));
      }
    }
  }

  if (lhs_types.size() != rhs_types.size()) {
    issues().Add(issues::kMismatchedAssignStmtValueCount, assign_stmt->start(),
                 "invalid operation: can not assign " + std::to_string(rhs_types.size()) +
                     " values to " + std::to_string(lhs_types.size()) + " operands");
  }
  for (size_t i = 0; i < lhs_types.size() && i < rhs_types.size(); i++) {
    types::Type* lhs_type = lhs_types.at(i);
    types::Type* rhs_type = rhs_types.at(i);

    if (assign_stmt->tok() == tokens::kDefine &&
        assign_stmt->lhs().at(i)->node_kind() == ast::NodeKind::kIdent) {
      ast::Ident* ident = static_cast<ast::Ident*>(assign_stmt->lhs().at(i));
      types::Object* obj = info()->DefinitionOf(ident);
      if (obj->object_kind() == types::ObjectKind::kVariable && rhs_type != nullptr) {
        if (rhs_type->type_kind() == types::TypeKind::kBasic) {
          types::Basic::Kind basic_kind = static_cast<types::Basic*>(rhs_type)->kind();
          types::Basic::Kind typed_basic_kind = types::ConvertIfUntyped(basic_kind);
          rhs_type = info()->basic_type(typed_basic_kind);
        }
        info_builder().SetObjectType(static_cast<types::Variable*>(obj), rhs_type);
      }
      continue;
    }

    if (lhs_type == nullptr || rhs_type == nullptr) {
      continue;
    } else if (!types::IsAssignableTo(rhs_type, lhs_type)) {
      if (assign_stmt->rhs().size() == assign_stmt->lhs().size()) {
        issues().Add(issues::kMismatchedAssignStmtValueType,
                     {assign_stmt->lhs().at(i)->start(), assign_stmt->rhs().at(i)->start()},
                     "can not assign value of type " +
                         rhs_type->ToString(types::StringRep::kShort) + " to operand of type " +
                         lhs_type->ToString(types::StringRep::kShort));
      } else {
        issues().Add(issues::kMismatchedAssignStmtValueType,
                     {assign_stmt->lhs().at(i)->start(), assign_stmt->rhs().at(0)->start()},
                     "can not assign value of type " +
                         rhs_type->ToString(types::StringRep::kShort) + " to operand of type " +
                         lhs_type->ToString(types::StringRep::kShort));
      }
    }
  }
}

void StmtHandler::CheckExprStmt(ast::ExprStmt* expr_stmt) {
  // TODO: check no value gets discarded.
  type_resolver().expr_handler().CheckExpr(expr_stmt->x());
}

void StmtHandler::CheckIncDecStmt(ast::IncDecStmt* inc_dec_stmt) {
  type_resolver().expr_handler().CheckIntegerExpr(inc_dec_stmt->x());
}

void StmtHandler::CheckReturnStmt(ast::ReturnStmt* return_stmt, Context ctx) {
  // TODO: check return stmt is last in block
  std::vector<ast::Expr*> result_exprs = return_stmt->results();
  if (result_exprs.empty()) {
    return;
  }
  std::vector<types::Type*> result_types =
      type_resolver().expr_handler().CheckValueExprs(result_exprs);
  if (result_types.empty()) {
    return;
  }

  if (result_exprs.size() == 1 && result_types.at(0)->type_kind() == types::TypeKind::kTuple) {
    types::Tuple* result_tuple = static_cast<types::Tuple*>(result_types.at(0));
    if (!types::IsAssignableTo(result_tuple, ctx.func_results)) {
      issues().Add(issues::kUnexpectedReturnStmtFuncCallOperandType, return_stmt->start(),
                   "invalid operation: results can not be assigned to function result types");
    }
    return;
  }
  if (ctx.func_results == nullptr || result_types.size() != ctx.func_results->variables().size()) {
    issues().Add(issues::kMismatchedReturnStmtOperandCount, return_stmt->start(),
                 "invalid operation: number of results does not match expected number of results");
    return;
  }
  for (size_t i = 0; i < result_types.size(); i++) {
    types::Type* expected_result_type = ctx.func_results->variables().at(i)->type();
    types::Type* given_result_type = result_types.at(i);
    ast::Expr* result_expr = return_stmt->results().at(i);
    if (!types::IsAssignableTo(given_result_type, expected_result_type)) {
      issues().Add(issues::kUnexpectedReturnStmtOperandType, result_expr->start(),
                   "invalid operation: result of type " +
                       given_result_type->ToString(types::StringRep::kShort) +
                       " can not be assigned to function result type " +
                       expected_result_type->ToString(types::StringRep::kShort));
    }
  }
  return;
}

void StmtHandler::CheckIfStmt(ast::IfStmt* if_stmt, Context ctx) {
  if (if_stmt->init_stmt()) {
    CheckStmt(if_stmt->init_stmt(), ctx);
  }
  type_resolver().expr_handler().CheckBoolExpr(if_stmt->cond_expr());

  ctx.can_fallthrough = false;
  CheckBlockStmt(if_stmt->body(), ctx);
  if (if_stmt->else_stmt()) {
    CheckStmt(if_stmt->else_stmt(), ctx);
  }
}

void StmtHandler::CheckExprSwitchStmt(ast::ExprSwitchStmt* switch_stmt, Context ctx) {
  if (switch_stmt->init_stmt()) {
    CheckStmt(switch_stmt->init_stmt(), ctx);
  }
  types::Type* tag_type = info()->basic_type(types::Basic::Kind::kUntypedBool);
  if (switch_stmt->tag_expr()) {
    tag_type = type_resolver().expr_handler().CheckValueExpr(switch_stmt->tag_expr());
  }
  ctx.can_break = true;
  bool seen_default = false;
  for (size_t i = 0; i < switch_stmt->body()->stmts().size(); i++) {
    ast::Stmt* stmt = switch_stmt->body()->stmts().at(i);
    ast::CaseClause* case_clause = static_cast<ast::CaseClause*>(stmt);
    if (case_clause->tok() == tokens::kDefault) {
      if (seen_default) {
        issues().Add(issues::kDuplicateDefaultCase, case_clause->start(),
                     "duplicate default case in switch statement");
      } else {
        seen_default = true;
      }
    }
    ctx.can_fallthrough = (i < switch_stmt->body()->stmts().size() - 1);
    CheckExprCaseClause(case_clause, tag_type, ctx);
  }
}

void StmtHandler::CheckExprCaseClause(ast::CaseClause* case_clause, types::Type* tag_type,
                                      Context ctx) {
  for (ast::Expr* expr : case_clause->cond_vals()) {
    types::Type* expr_type = type_resolver().expr_handler().CheckValueExpr(expr);
    if (expr_type == nullptr || tag_type == nullptr) {
      continue;
    }
    if (!types::IsComparable(tag_type, expr_type)) {
      issues().Add(issues::kUnexpectedExprCaseValueType, expr->start(),
                   "invalid operation: can not compare value expression with switch tag");
    }
  }
  for (size_t i = 0; i < case_clause->body().size(); i++) {
    ast::Stmt* stmt = case_clause->body().at(i);
    ctx.is_last_stmt_in_block = (i == case_clause->body().size());
    CheckStmt(stmt, ctx);
  }
}

void StmtHandler::CheckTypeSwitchStmt(ast::TypeSwitchStmt* switch_stmt, Context ctx) {
  types::Type* tag_type = type_resolver().expr_handler().CheckValueExpr(switch_stmt->tag_expr());
  ctx.can_break = true;
  ctx.can_fallthrough = false;
  bool seen_default = false;
  for (size_t i = 0; i < switch_stmt->body()->stmts().size(); i++) {
    ast::Stmt* stmt = switch_stmt->body()->stmts().at(i);
    ast::CaseClause* case_clause = static_cast<ast::CaseClause*>(stmt);
    if (case_clause->tok() == tokens::kDefault) {
      if (seen_default) {
        issues().Add(issues::kDuplicateDefaultCase, case_clause->start(),
                     "duplicate default case in switch statement");
      } else {
        seen_default = true;
      }
    }
    CheckTypeCaseClause(case_clause, tag_type, ctx);
  }
}

void StmtHandler::CheckTypeCaseClause(ast::CaseClause* case_clause, types::Type* tag_type,
                                      Context ctx) {
  types::Type* implicit_tag_type = tag_type;
  for (ast::Expr* expr : case_clause->cond_vals()) {
    types::Type* expr_type = type_resolver().type_handler().EvaluateTypeExpr(expr);
    if (expr_type == nullptr) {
      continue;
    }
    if (tag_type != nullptr && !types::IsAssertableTo(tag_type, expr_type)) {
      issues().Add(issues::kTypeSwitchCaseNeverPossible, expr->start(),
                   "invalid operation: value of type switch tag can never have the given type");
      continue;
    }
    if (case_clause->cond_vals().size() == 1) {
      implicit_tag_type = expr_type;
    }
  }
  types::Variable* implicit_tag = static_cast<types::Variable*>(info()->ImplicitOf(case_clause));
  info_builder().SetObjectType(implicit_tag, implicit_tag_type);

  for (size_t i = 0; i < case_clause->body().size(); i++) {
    ast::Stmt* stmt = case_clause->body().at(i);
    ctx.is_last_stmt_in_block = (i == case_clause->body().size());
    CheckStmt(stmt, ctx);
  }
}

void StmtHandler::CheckForStmt(ast::ForStmt* for_stmt, Context ctx) {
  if (for_stmt->init_stmt() != nullptr) {
    CheckStmt(for_stmt->init_stmt(), ctx);
  }
  type_resolver().expr_handler().CheckBoolExpr(for_stmt->cond_expr());
  if (for_stmt->post_stmt()) {
    CheckStmt(for_stmt->post_stmt(), ctx);
  }

  ctx.can_break = true;
  ctx.can_continue = true;
  ctx.can_fallthrough = false;
  CheckBlockStmt(for_stmt->body(), ctx);
}

void StmtHandler::CheckBranchStmt(ast::BranchStmt* branch_stmt, Context ctx) {
  if (!ctx.is_last_stmt_in_block) {
    issues().Add(issues::kUnexpectedBranchStmtBeforeBlockEnd, branch_stmt->start(),
                 "branch statement is not last in block");
    return;
  }
  ast::Stmt* labeled_destination = nullptr;
  bool destination_is_labeled_loop = false;
  bool destination_is_labeled_switch = false;
  if (branch_stmt->label() != nullptr) {
    if (branch_stmt->tok() == tokens::kFallthrough) {
      issues().Add(issues::kUnexpectedFallthroughStmtLabel, branch_stmt->start(),
                   "fallthrough with label is now allowed");
      return;
    }
    auto it = ctx.labels.find(branch_stmt->label()->name());
    if (it == ctx.labels.end()) {
      // TODO: warn either here or in identifer resolver (not both)
      issues().Add(issues::kUnresolvedBranchStmtLabel, branch_stmt->start(),
                   "branch label does not refer to any enclosing statement");
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
          issues().Add(issues::kUnexpectedBreakStmt, branch_stmt->start(),
                       "can not break: no enclosing switch or for statement");
        }
      } else if (!destination_is_labeled_loop && !destination_is_labeled_switch) {
        issues().Add(issues::kUnexpectedBreakStmtLabel, branch_stmt->start(),
                     "break label does not refer to an enclosing switch or for statement");
      }
      return;
    case tokens::kContinue:
      if (labeled_destination == nullptr) {
        if (!ctx.can_continue) {
          issues().Add(issues::kUnexpectedContinueStmt, branch_stmt->start(),
                       "can not continue: no enclosing for statement");
        }
      } else if (!destination_is_labeled_loop) {
        issues().Add(issues::kUnexpectedContinueStmtLabel, branch_stmt->start(),
                     "continue label does not refer to an enclosing for statement");
      }
      return;
    case tokens::kFallthrough:
      if (!ctx.can_fallthrough) {
        issues().Add(issues::kUnexpectedFallthroughStmt, branch_stmt->start(),
                     "can not fallthrough: no expression type switch case immedately after");
      }
      return;
    default:
      throw "internal error: unexpected branch statement";
  }
}

}  // namespace type_checker
}  // namespace lang

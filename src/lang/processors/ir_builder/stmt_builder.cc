//
//  stmt_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 7/18/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "stmt_builder.h"

namespace lang {
namespace ir_builder {

void StmtBuilder::BuildBlockStmt(ast::BlockStmt* block_stmt, Context& ctx) {
  for (auto stmt : block_stmt->stmts()) {
    BuildStmt(stmt, ctx);
  }
}

void StmtBuilder::BuildStmt(ast::Stmt* stmt, Context& ctx) {
  while (stmt->node_kind() == ast::NodeKind::kLabeledStmt) {
    stmt = static_cast<ast::LabeledStmt*>(stmt)->stmt();
  }
  switch (stmt->node_kind()) {
    case ast::NodeKind::kBlockStmt:
      BuildBlockStmt(static_cast<ast::BlockStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kDeclStmt:
      BuildDeclStmt(static_cast<ast::DeclStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kAssignStmt:
      BuildAssignStmt(static_cast<ast::AssignStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kExprStmt:
      BuildExprStmt(static_cast<ast::ExprStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kIncDecStmt:
      BuildIncDecStmt(static_cast<ast::IncDecStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kReturnStmt:
      BuildReturnStmt(static_cast<ast::ReturnStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kIfStmt:
      BuildIfStmt(static_cast<ast::IfStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kExprSwitchStmt:
      BuildExprSwitchStmt(static_cast<ast::ExprSwitchStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kTypeSwitchStmt:
      BuildTypeSwitchStmt(static_cast<ast::TypeSwitchStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kForStmt:
      BuildForStmt(static_cast<ast::ForStmt*>(stmt), ctx);
      break;
    case ast::NodeKind::kBranchStmt:
      BuildBranchStmt(static_cast<ast::BranchStmt*>(stmt), ctx);
      break;
    default:
      throw "internal error: unexpected stmt";
  }
}

void StmtBuilder::BuildDeclStmt(ast::DeclStmt* decl_stmt, Context& ctx) {
  ast::GenDecl* decl = decl_stmt->decl();
  switch (decl->tok()) {
    case tokens::kImport:
    case tokens::kConst:
    case tokens::kType:
      return;
    case tokens::kVar:
      break;
    default:
      throw "internal error: unexpected decl";
  }
  for (ast::Spec* spec : decl->specs()) {
    ast::ValueSpec* value_spec = static_cast<ast::ValueSpec*>(spec);
    for (ast::Ident* name : value_spec->names()) {
      types::Variable* var = static_cast<types::Variable*>(type_info_->DefinitionOf(name));
      if (var == nullptr) {
        continue;
      }
      BuildVarDecl(var, ctx);
    }

    if (value_spec->values().empty()) {
      for (ast::Ident* name : value_spec->names()) {
        types::Variable* var = static_cast<types::Variable*>(type_info_->DefinitionOf(name));
        if (var == nullptr) {
          continue;
        }
        std::shared_ptr<ir::Value> address = ctx.LookupAddressOfVar(var);
        std::shared_ptr<ir::Value> default_value = expr_builder_.DefaultIRValueForType(var->type());
        ctx.block()->instrs().push_back(std::make_unique<ir::StoreInstr>(address, default_value));
      }
    } else {
      std::vector<std::shared_ptr<ir::Value>> values =
          expr_builder_.BuildValuesOfExprs(value_spec->values(), ctx);
      for (size_t i = 0; i < value_spec->names().size() && i < values.size(); i++) {
        ast::Ident* name = value_spec->names().at(i);
        types::Variable* var = static_cast<types::Variable*>(type_info_->DefinitionOf(name));
        if (var == nullptr) {
          continue;
        }
        std::shared_ptr<ir::Value> address = ctx.LookupAddressOfVar(var);
        std::shared_ptr<ir::Value> value = values.at(i);
        ctx.block()->instrs().push_back(std::make_unique<ir::StoreInstr>(address, value));
      }
    }
  }
}

void StmtBuilder::BuildAssignStmt(ast::AssignStmt* assign_stmt, Context& ctx) {
  if (assign_stmt->tok() == tokens::kDefine) {
    for (ast::Expr* lhs : assign_stmt->lhs()) {
      if (lhs->node_kind() != ast::NodeKind::kIdent) {
        continue;
      }
      ast::Ident* ident = static_cast<ast::Ident*>(lhs);
      types::Variable* var = static_cast<types::Variable*>(type_info_->DefinitionOf(ident));
      if (var == nullptr) {
        continue;
      }
      BuildVarDecl(var, ctx);
    }
  }

  std::vector<std::shared_ptr<ir::Computed>> lhs_addresses =
      expr_builder_.BuildAddressesOfExprs(assign_stmt->lhs(), ctx);
  std::vector<std::shared_ptr<ir::Value>> rhs_values =
      expr_builder_.BuildValuesOfExprs(assign_stmt->rhs(), ctx);

  switch (assign_stmt->tok()) {
    case tokens::kAssign:
    case tokens::kDefine:
      for (size_t i = 0; i < lhs_addresses.size(); i++) {
        std::shared_ptr<ir::Value> lhs_address = lhs_addresses.at(i);
        std::shared_ptr<ir::Value> rhs_value = rhs_values.at(i);
        ctx.block()->instrs().push_back(std::make_unique<ir::StoreInstr>(lhs_address, rhs_value));
      }
      break;
    case tokens::kAddAssign:
    case tokens::kSubAssign:
    case tokens::kMulAssign:
    case tokens::kQuoAssign:
    case tokens::kRemAssign:
    case tokens::kAndAssign:
    case tokens::kOrAssign:
    case tokens::kXorAssign:
    case tokens::kShlAssign:
    case tokens::kShrAssign:
    case tokens::kAndNotAssign:
      // TODO: implement
      break;
    default:
      throw "internal error: unexpected assign op";
  }
}

void StmtBuilder::BuildExprStmt(ast::ExprStmt* expr_stmt, Context& ctx) {
  expr_builder_.BuildValuesOfExpr(expr_stmt->x(), ctx);
}

void StmtBuilder::BuildIncDecStmt(ast::IncDecStmt* inc_dec_stmt, Context& ctx) {
  // TODO: implement
}

void StmtBuilder::BuildReturnStmt(ast::ReturnStmt* return_stmt, Context& ctx) {
  std::vector<std::shared_ptr<ir::Value>> results =
      expr_builder_.BuildValuesOfExprs(return_stmt->results(), ctx);

  ctx.block()->instrs().push_back(std::make_unique<ir::ReturnInstr>(results));
}

void StmtBuilder::BuildIfStmt(ast::IfStmt* if_stmt, Context& ctx) {
  if (if_stmt->init_stmt() != nullptr) {
    BuildStmt(if_stmt->init_stmt(), ctx);
  }
  std::shared_ptr<ir::Value> condition =
      expr_builder_.BuildValuesOfExpr(if_stmt->cond_expr(), ctx).front();

  ir::Block* start_block = ctx.block();

  ir::Block* if_entry_block = ctx.func()->AddBlock();
  Context if_ctx = ctx.SubContextForBlock(if_entry_block);
  BuildBlockStmt(if_stmt->body(), if_ctx);
  ir::Block* if_exit_block = if_ctx.block();

  bool has_else = if_stmt->else_stmt() != nullptr;
  ir::Block* else_entry_block = nullptr;
  ir::Block* else_exit_block = nullptr;
  std::optional<Context> else_ctx;
  if (has_else) {
    else_entry_block = ctx.func()->AddBlock();
    else_ctx = ctx.SubContextForBlock(else_entry_block);
    BuildStmt(if_stmt->else_stmt(), else_ctx.value());
    else_exit_block = else_ctx->block();
  }

  ir::Block* merge_block = ctx.func()->AddBlock();
  ctx.set_block(merge_block);

  ir::block_num_t destination_true = if_entry_block->number();
  ir::block_num_t destination_false =
      (has_else) ? else_entry_block->number() : merge_block->number();
  start_block->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(condition, destination_true, destination_false));
  if_exit_block->instrs().push_back(std::make_unique<ir::JumpInstr>(merge_block->number()));
  if (has_else) {
    else_exit_block->instrs().push_back(std::make_unique<ir::JumpInstr>(merge_block->number()));
  }

  ctx.func()->AddControlFlow(start_block->number(), if_entry_block->number());
  ctx.func()->AddControlFlow(if_exit_block->number(), merge_block->number());
  if (has_else) {
    ctx.func()->AddControlFlow(start_block->number(), else_entry_block->number());
    ctx.func()->AddControlFlow(else_exit_block->number(), merge_block->number());
  } else {
    ctx.func()->AddControlFlow(start_block->number(), merge_block->number());
  }
}

void StmtBuilder::BuildExprSwitchStmt(ast::ExprSwitchStmt* expr_switch_stmt, Context& ctx) {
  // TODO: implement
}

void StmtBuilder::BuildTypeSwitchStmt(ast::TypeSwitchStmt* type_switch_stmt, Context& ctx) {
  // TODO: implement
}

void StmtBuilder::BuildForStmt(ast::ForStmt* for_stmt, Context& ctx) {
  if (for_stmt->init_stmt() != nullptr) {
    BuildStmt(for_stmt->init_stmt(), ctx);
  }

  ir::Block* start_block = ctx.block();

  ir::Block* body_entry_block = ctx.func()->AddBlock();
  Context body_ctx = ctx.SubContextForBlock(body_entry_block);
  BuildBlockStmt(for_stmt->body(), body_ctx);
  if (for_stmt->post_stmt() != nullptr) {
    BuildStmt(for_stmt->post_stmt(), body_ctx);
  }
  ir::Block* body_exit_block = body_ctx.block();

  ir::Block* continue_block = ctx.func()->AddBlock();

  ir::Block* header_block = ctx.func()->AddBlock();
  Context header_ctx = ctx.SubContextForBlock(header_block);
  std::shared_ptr<ir::Value> condition =
      expr_builder_.BuildValuesOfExpr(for_stmt->cond_expr(), header_ctx).front();

  header_block->instrs().push_back(std::make_unique<ir::JumpCondInstr>(
      condition, body_entry_block->number(), continue_block->number()));
  start_block->instrs().push_back(std::make_unique<ir::JumpInstr>(header_block->number()));
  body_exit_block->instrs().push_back(std::make_unique<ir::JumpInstr>(header_block->number()));

  ctx.func()->AddControlFlow(start_block->number(), header_block->number());
  ctx.func()->AddControlFlow(header_block->number(), body_entry_block->number());
  ctx.func()->AddControlFlow(header_block->number(), continue_block->number());
  ctx.func()->AddControlFlow(body_exit_block->number(), header_block->number());

  ctx.set_block(continue_block);
}

void StmtBuilder::BuildBranchStmt(ast::BranchStmt* branch_stmt, Context& ctx) {
  // TODO: implement
}

void StmtBuilder::BuildVarDecl(types::Variable* var, Context& ctx) {
  const ir_ext::SharedPointer* pointer_type = type_builder_.BuildStrongPointerToType(var->type());
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(pointer_type, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir_ext::MakeSharedPointerInstr>(result));
  ctx.AddAddressOfVar(var, result);
}

void StmtBuilder::BuildVarDeletion(types::Variable* var, Context& ctx) {
  std::shared_ptr<ir::Computed> address = ctx.LookupAddressOfVar(var);
  ctx.block()->instrs().push_back(std::make_unique<ir_ext::DeleteSharedPointerInstr>(address));
}

}  // namespace ir_builder
}  // namespace lang

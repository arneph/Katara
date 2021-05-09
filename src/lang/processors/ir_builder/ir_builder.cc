//
//  ir_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 4/18/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "ir_builder.h"

#include <optional>

namespace lang {
namespace ir_builder {

std::unique_ptr<ir::Program> IRBuilder::TranslateProgram(packages::Package* main_package) {
  auto prog = std::make_unique<ir::Program>();
  auto builder = std::unique_ptr<IRBuilder>(new IRBuilder(prog));

  for (auto [file_name, file] : main_package->ast_package()->files()) {
    builder->PrepareDeclsInFile(file);
  }
  for (auto [file_name, file] : main_package->ast_package()->files()) {
    builder->BuildDeclsInFile(file);
  }

  return prog;
}

void IRBuilder::PrepareDeclsInFile(ast::File* file) {
  for (auto decl : file->decls()) {
    if (decl->node_kind() == ast::NodeKind::kGenDecl) {
      ast::GenDecl* gen_decl = static_cast<ast::GenDecl*>(decl);
      if (gen_decl->tok() == tokens::kImport) {
        continue;
      } else {
        continue;  // TODO: implement
      }
    } else if (decl->node_kind() == ast::NodeKind::kFuncDecl) {
      PrepareFuncDecl(static_cast<ast::FuncDecl*>(decl));
    } else {
      throw "not implemented";
    }
  }
}

void IRBuilder::PrepareFuncDecl(ast::FuncDecl* func_decl) {
  ir::Func* func = program_->AddFunc();
  func->set_name(func_decl->name()->name());
  funcs_.insert({func_decl->start(), func});
}

void IRBuilder::BuildDeclsInFile(ast::File* file) {
  for (auto decl : file->decls()) {
    if (decl->node_kind() == ast::NodeKind::kGenDecl) {
      ast::GenDecl* gen_decl = static_cast<ast::GenDecl*>(decl);
      if (gen_decl->tok() == tokens::kImport) {
        continue;
      } else {
        continue;  // TODO: implement
      }
    } else if (decl->node_kind() == ast::NodeKind::kFuncDecl) {
      BuildFuncDecl(static_cast<ast::FuncDecl*>(decl));
    } else {
      throw "internal error: unexpected decl";
    }
  }
}

void IRBuilder::BuildFuncDecl(ast::FuncDecl* func_decl) {
  ir::Func* func = funcs_.at(func_decl->start());
  ir::Block* entry_block = func->AddBlock();
  func->set_entry_block_num(entry_block->number());
  Context ctx(func);
  // TODO: handle args and results
  BuildBlockStmt(func_decl->body(), ctx);
}

void IRBuilder::BuildBlockStmt(ast::BlockStmt* block_stmt, Context& ctx) {
  for (auto stmt : block_stmt->stmts()) {
    BuildStmt(stmt, ctx);
  }
}

void IRBuilder::BuildStmt(ast::Stmt* stmt, Context& ctx) {
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

void IRBuilder::BuildDeclStmt(ast::DeclStmt* decl_stmt, Context& ctx) {
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
    // TODO: implement
  }
}

void IRBuilder::BuildAssignStmt(ast::AssignStmt* assign_stmt, Context& ctx) {
  // TODO: Implement
}

void IRBuilder::BuildExprStmt(ast::ExprStmt* expr_stmt, Context& ctx) {
  BuildExpr(expr_stmt->x(), ctx);
}

void IRBuilder::BuildIncDecStmt(ast::IncDecStmt* inc_dec_stmt, Context& ctx) {
  // TODO: implement
}

void IRBuilder::BuildReturnStmt(ast::ReturnStmt* return_stmt, Context& ctx) {
  std::vector<std::shared_ptr<ir::Value>> results = BuildExprs(return_stmt->results(), ctx);
}

void IRBuilder::BuildIfStmt(ast::IfStmt* if_stmt, Context& ctx) {
  if (if_stmt->init_stmt() != nullptr) {
    BuildStmt(if_stmt->init_stmt(), ctx);
  }
  std::shared_ptr<ir::Value> condition = BuildExpr(if_stmt->cond_expr(), ctx).front();

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

    BuildPhiInstrsForMerge({&if_ctx, &else_ctx.value()}, ctx);
  } else {
    ctx.func()->AddControlFlow(start_block->number(), merge_block->number());

    BuildPhiInstrsForMerge({&if_ctx, &ctx}, ctx);
  }
}

void IRBuilder::BuildExprSwitchStmt(ast::ExprSwitchStmt* expr_switch_stmt, Context& ctx) {
  // TODO: implement
}

void IRBuilder::BuildTypeSwitchStmt(ast::TypeSwitchStmt* type_switch_stmt, Context& ctx) {
  // TODO: implement
}

void IRBuilder::BuildForStmt(ast::ForStmt* for_stmt, Context& ctx) {
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
  BuildPhiInstrsForMerge({&ctx, &body_ctx}, header_ctx);
  std::shared_ptr<ir::Value> condition = BuildExpr(for_stmt->cond_expr(), header_ctx).front();

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

void IRBuilder::BuildBranchStmt(ast::BranchStmt* branch_stmt, Context& ctx) {
  // TODO: implement
}

void IRBuilder::BuildPhiInstrsForMerge(std::vector<Context*> input_ctxs, Context& phi_ctx) {
  // TODO: implement
}

std::vector<std::shared_ptr<ir::Value>> IRBuilder::BuildExprs(std::vector<ast::Expr*> exprs,
                                                              Context& ctx) {
  std::vector<std::shared_ptr<ir::Value>> values;
  for (auto expr : exprs) {
    std::vector<std::shared_ptr<ir::Value>> expr_values = BuildExpr(expr, ctx);
    if (!expr_values.empty()) {
      values.push_back(expr_values.front());
    }
  }
  return values;
}

std::vector<std::shared_ptr<ir::Value>> IRBuilder::BuildExpr(ast::Expr* expr, Context& ctx) {
  return {std::make_shared<ir::Constant>(
      program_->atomic_type_table().AtomicTypeForKind(ir::AtomicTypeKind::kBool),
      1)};  // TODO: remove
  switch (expr->node_kind()) {
    case ast::NodeKind::kUnaryExpr:
      return {BuildUnaryExpr(static_cast<ast::UnaryExpr*>(expr), ctx)};
    case ast::NodeKind::kBinaryExpr:
      return {BuildBinaryExpr(static_cast<ast::BinaryExpr*>(expr), ctx)};
    case ast::NodeKind::kCompareExpr:
      return {BuildCompareExpr(static_cast<ast::CompareExpr*>(expr), ctx)};
    case ast::NodeKind::kParenExpr:
      return BuildExpr(static_cast<ast::ParenExpr*>(expr)->x(), ctx);
    case ast::NodeKind::kSelectionExpr:
      return BuildSelectionExpr(static_cast<ast::SelectionExpr*>(expr), ctx);
    case ast::NodeKind::kTypeAssertExpr:
      return BuildTypeAssertExpr(static_cast<ast::TypeAssertExpr*>(expr), ctx);
    case ast::NodeKind::kIndexExpr:
      return {BuildIndexExpr(static_cast<ast::IndexExpr*>(expr), ctx)};
    case ast::NodeKind::kCallExpr:
      return BuildCallExpr(static_cast<ast::CallExpr*>(expr), ctx);
    case ast::NodeKind::kFuncLit:
      return {BuildFuncLit(static_cast<ast::FuncLit*>(expr), ctx)};
    case ast::NodeKind::kCompositeLit:
      return {BuildCompositeLit(static_cast<ast::CompositeLit*>(expr), ctx)};
    default:
      throw "internal error: unexpected expr";
  }
}

std::shared_ptr<ir::Value> IRBuilder::BuildUnaryExpr(ast::UnaryExpr* expr, Context& ctx) {
  // TODO: implement
  return nullptr;
}

std::shared_ptr<ir::Value> IRBuilder::BuildBinaryExpr(ast::BinaryExpr* expr, Context& ctx) {
  // TODO: implement
  return nullptr;
}

std::shared_ptr<ir::Value> IRBuilder::BuildCompareExpr(ast::CompareExpr* expr, Context& ctx) {
  // TODO: implement
  return nullptr;
}

std::vector<std::shared_ptr<ir::Value>> IRBuilder::BuildSelectionExpr(ast::SelectionExpr* expr,
                                                                      Context& ctx) {
  // TODO: implement
  return {};
}

std::vector<std::shared_ptr<ir::Value>> IRBuilder::BuildTypeAssertExpr(ast::TypeAssertExpr* expr,
                                                                       Context& ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Value> IRBuilder::BuildIndexExpr(ast::IndexExpr* expr, Context& ctx) {
  // TODO: implement
  return {};
}

std::vector<std::shared_ptr<ir::Value>> IRBuilder::BuildCallExpr(ast::CallExpr* expr,
                                                                 Context& ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Constant> IRBuilder::BuildFuncLit(ast::FuncLit* expr, Context& ctx) {
  // TODO: implement
  return nullptr;
}

std::shared_ptr<ir::Value> IRBuilder::BuildCompositeLit(ast::CompositeLit* expr, Context& ctx) {
  // TODO: implement
  return nullptr;
}

std::shared_ptr<ir::Value> IRBuilder::BuildBasicLit(ast::BasicLit* basic_lit, Context& ctx) {
  // TODO: implement
  return nullptr;
}

std::shared_ptr<ir::Value> IRBuilder::BuildIdent(ast::Ident* ident, Context& ctx) {
  // TODO: implement
  return nullptr;
}

}  // namespace ir_builder
}  // namespace lang

//
//  stmt_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 7/18/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "stmt_builder.h"

#include "src/common/logging.h"

namespace lang {
namespace ir_builder {

void StmtBuilder::BuildBlockStmt(ast::BlockStmt* block_stmt, ASTContext& ast_ctx,
                                 IRContext& ir_ctx) {
  ASTContext child_ast_ctx = ast_ctx.ChildContextFor(block_stmt);
  for (auto stmt : block_stmt->stmts()) {
    BuildStmt(stmt, child_ast_ctx, ir_ctx);
  }
  if (ir_ctx.block()->instrs().empty() ||
      (ir_ctx.block()->instrs().back()->instr_kind() != ir::InstrKind::kJump &&
       ir_ctx.block()->instrs().back()->instr_kind() != ir::InstrKind::kJumpCond &&
       ir_ctx.block()->instrs().back()->instr_kind() != ir::InstrKind::kReturn)) {
    BuildVarDeletionsForASTContext(&child_ast_ctx, ir_ctx);
  }
}

void StmtBuilder::BuildVarDecl(types::Variable* var, bool initialize_var, ASTContext& ast_ctx,
                               IRContext& ir_ctx) {
  const ir_ext::SharedPointer* pointer_type = type_builder_.BuildStrongPointerToType(var->type());
  std::shared_ptr<ir::Computed> address =
      std::make_shared<ir::Computed>(pointer_type, ir_ctx.func()->next_computed_number());
  ir_ctx.block()->instrs().push_back(std::make_unique<ir_ext::MakeSharedPointerInstr>(address));
  ast_ctx.AddAddressOfVar(var, address);

  if (initialize_var) {
    std::shared_ptr<ir::Value> default_value = value_builder_.BuildDefaultForType(var->type());
    ir_ctx.block()->instrs().push_back(std::make_unique<ir::StoreInstr>(address, default_value));
  }
}

void StmtBuilder::BuildVarDeletionsForASTContextAndParents(ASTContext* ast_ctx, IRContext& ir_ctx) {
  for (ASTContext* ctx = ast_ctx; ctx != nullptr; ctx = ctx->parent()) {
    BuildVarDeletionsForASTContext(ctx, ir_ctx);
  }
}

void StmtBuilder::BuildVarDeletionsForASTContext(ASTContext* ast_ctx, IRContext& ir_ctx) {
  for (auto it = ast_ctx->var_addresses().rbegin(); it != ast_ctx->var_addresses().rend(); ++it) {
    std::shared_ptr<ir::Computed> address = it->second;
    ir_ctx.block()->instrs().push_back(std::make_unique<ir_ext::DeleteSharedPointerInstr>(address));
  }
}

void StmtBuilder::BuildStmt(ast::Stmt* stmt, ASTContext& ast_ctx, IRContext& ir_ctx) {
  while (stmt->node_kind() == ast::NodeKind::kLabeledStmt) {
    stmt = static_cast<ast::LabeledStmt*>(stmt)->stmt();
  }
  switch (stmt->node_kind()) {
    case ast::NodeKind::kBlockStmt:
      BuildBlockStmt(static_cast<ast::BlockStmt*>(stmt), ast_ctx, ir_ctx);
      break;
    case ast::NodeKind::kDeclStmt:
      BuildDeclStmt(static_cast<ast::DeclStmt*>(stmt), ast_ctx, ir_ctx);
      break;
    case ast::NodeKind::kAssignStmt:
      BuildAssignStmt(static_cast<ast::AssignStmt*>(stmt), ast_ctx, ir_ctx);
      break;
    case ast::NodeKind::kExprStmt:
      BuildExprStmt(static_cast<ast::ExprStmt*>(stmt), ast_ctx, ir_ctx);
      break;
    case ast::NodeKind::kIncDecStmt:
      BuildIncDecStmt(static_cast<ast::IncDecStmt*>(stmt), ast_ctx, ir_ctx);
      break;
    case ast::NodeKind::kReturnStmt:
      BuildReturnStmt(static_cast<ast::ReturnStmt*>(stmt), ast_ctx, ir_ctx);
      break;
    case ast::NodeKind::kIfStmt:
      BuildIfStmt(static_cast<ast::IfStmt*>(stmt), ast_ctx, ir_ctx);
      break;
    case ast::NodeKind::kExprSwitchStmt:
      BuildExprSwitchStmt(static_cast<ast::ExprSwitchStmt*>(stmt), ast_ctx, ir_ctx);
      break;
    case ast::NodeKind::kTypeSwitchStmt:
      BuildTypeSwitchStmt(static_cast<ast::TypeSwitchStmt*>(stmt), ast_ctx, ir_ctx);
      break;
    case ast::NodeKind::kForStmt:
      BuildForStmt(static_cast<ast::ForStmt*>(stmt), ast_ctx, ir_ctx);
      break;
    case ast::NodeKind::kBranchStmt:
      BuildBranchStmt(static_cast<ast::BranchStmt*>(stmt), ast_ctx, ir_ctx);
      break;
    default:
      common::fail("unexpected stmt");
  }
}

void StmtBuilder::BuildDeclStmt(ast::DeclStmt* decl_stmt, ASTContext& ast_ctx, IRContext& ir_ctx) {
  ast::GenDecl* decl = decl_stmt->decl();
  switch (decl->tok()) {
    case tokens::kImport:
    case tokens::kConst:
    case tokens::kType:
      return;
    case tokens::kVar:
      break;
    default:
      common::fail("unexpected decl");
  }
  for (ast::Spec* spec : decl->specs()) {
    ast::ValueSpec* value_spec = static_cast<ast::ValueSpec*>(spec);
    for (ast::Ident* name : value_spec->names()) {
      types::Variable* var = static_cast<types::Variable*>(type_info_->DefinitionOf(name));
      if (var == nullptr) {
        continue;
      }
      BuildVarDecl(var, /*initialize_var=*/value_spec->values().empty(), ast_ctx, ir_ctx);
    }

    if (!value_spec->values().empty()) {
      std::vector<std::shared_ptr<ir::Value>> values =
          expr_builder_.BuildValuesOfExprs(value_spec->values(), ast_ctx, ir_ctx);
      for (size_t i = 0; i < value_spec->names().size() && i < values.size(); i++) {
        ast::Ident* name = value_spec->names().at(i);
        types::Variable* var = static_cast<types::Variable*>(type_info_->DefinitionOf(name));
        if (var == nullptr) {
          continue;
        }
        std::shared_ptr<ir::Value> address = ast_ctx.LookupAddressOfVar(var);
        std::shared_ptr<ir::Value> value = values.at(i);
        ir_ctx.block()->instrs().push_back(std::make_unique<ir::StoreInstr>(address, value));
      }
    }
  }
}

void StmtBuilder::BuildAssignStmt(ast::AssignStmt* assign_stmt, ASTContext& ast_ctx,
                                  IRContext& ir_ctx) {
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
      BuildVarDecl(var, /*initialize_var=*/false, ast_ctx, ir_ctx);
    }
  }

  std::vector<std::shared_ptr<ir::Computed>> lhs_addresses =
      expr_builder_.BuildAddressesOfExprs(assign_stmt->lhs(), ast_ctx, ir_ctx);
  std::vector<std::shared_ptr<ir::Value>> rhs_values =
      expr_builder_.BuildValuesOfExprs(assign_stmt->rhs(), ast_ctx, ir_ctx);

  switch (assign_stmt->tok()) {
    case tokens::kAssign:
    case tokens::kDefine:
      BuildSimpleAssignStmt(lhs_addresses, rhs_values, ir_ctx);
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
      BuildOpAssignStmt(assign_stmt->tok(), lhs_addresses, rhs_values, ir_ctx);
      break;
    default:
      common::fail("unexpected assign op");
  }
}

void StmtBuilder::BuildSimpleAssignStmt(std::vector<std::shared_ptr<ir::Computed>> lhs_addresses,
                                        std::vector<std::shared_ptr<ir::Value>> rhs_values,
                                        IRContext& ir_ctx) {
  for (size_t i = 0; i < lhs_addresses.size(); i++) {
    std::shared_ptr<ir::Value> lhs_address = lhs_addresses.at(i);
    std::shared_ptr<ir::Value> rhs_value = rhs_values.at(i);
    ir_ctx.block()->instrs().push_back(std::make_unique<ir::StoreInstr>(lhs_address, rhs_value));
  }
}

void StmtBuilder::BuildOpAssignStmt(tokens::Token op_assign_tok,
                                    std::vector<std::shared_ptr<ir::Computed>> lhs_addresses,
                                    std::vector<std::shared_ptr<ir::Value>> rhs_values,
                                    IRContext& ir_ctx) {
  std::vector<std::shared_ptr<ir::Value>> assigned_values;
  assigned_values.reserve(lhs_addresses.size());
  for (size_t i = 0; i < lhs_addresses.size(); i++) {
    std::shared_ptr<ir::Value> lhs_address = lhs_addresses.at(i);
    auto lhs_pointer_type = static_cast<const ir_ext::SharedPointer*>(lhs_address->type());
    const ir::Type* lhs_type = lhs_pointer_type->element();
    std::shared_ptr<ir::Computed> lhs_value =
        std::make_shared<ir::Computed>(lhs_type, ir_ctx.func()->next_computed_number());
    ir_ctx.block()->instrs().push_back(std::make_unique<ir::LoadInstr>(lhs_value, lhs_address));
    std::shared_ptr<ir::Value> rhs_value = rhs_values.at(i);

    if (op_assign_tok == tokens::kAddAssign && lhs_type->type_kind() == ir::TypeKind::kLangString) {
      assigned_values.push_back(value_builder_.BuildStringConcat(lhs_value, rhs_value, ir_ctx));

    } else if (op_assign_tok == tokens::kShlAssign || op_assign_tok == tokens::kShrAssign) {
      common::Int::ShiftOp op = [op_assign_tok]() {
        switch (op_assign_tok) {
          case tokens::kShlAssign:
            return common::Int::ShiftOp::kLeft;
          case tokens::kShrAssign:
            return common::Int::ShiftOp::kRight;
          default:
            common::fail("unexpected assign op");
        }
      }();
      rhs_value = value_builder_.BuildConversion(rhs_value, &ir::kU64, ir_ctx);
      assigned_values.push_back(value_builder_.BuildIntShiftOp(lhs_value, op, rhs_value, ir_ctx));

    } else {
      common::Int::BinaryOp op = [op_assign_tok]() {
        switch (op_assign_tok) {
          case tokens::kAddAssign:
            return common::Int::BinaryOp::kAdd;
          case tokens::kSubAssign:
            return common::Int::BinaryOp::kSub;
          case tokens::kMulAssign:
            return common::Int::BinaryOp::kMul;
          case tokens::kQuoAssign:
            return common::Int::BinaryOp::kDiv;
          case tokens::kRemAssign:
            return common::Int::BinaryOp::kRem;
          case tokens::kAndAssign:
            return common::Int::BinaryOp::kAnd;
          case tokens::kOrAssign:
            return common::Int::BinaryOp::kOr;
          case tokens::kXorAssign:
            return common::Int::BinaryOp::kXor;
          case tokens::kAndNotAssign:
            return common::Int::BinaryOp::kAndNot;
          default:
            common::fail("unexpected assign op");
        }
      }();
      rhs_value = value_builder_.BuildConversion(rhs_value, lhs_type, ir_ctx);
      assigned_values.push_back(value_builder_.BuildIntBinaryOp(lhs_value, op, rhs_value, ir_ctx));
    }
  }

  for (size_t i = 0; i < lhs_addresses.size(); i++) {
    std::shared_ptr<ir::Value> address = lhs_addresses.at(i);
    std::shared_ptr<ir::Value> value = assigned_values.at(i);
    ir_ctx.block()->instrs().push_back(std::make_unique<ir::StoreInstr>(address, value));
  }
}

void StmtBuilder::BuildExprStmt(ast::ExprStmt* expr_stmt, ASTContext& ast_ctx, IRContext& ir_ctx) {
  expr_builder_.BuildValuesOfExpr(expr_stmt->x(), ast_ctx, ir_ctx);
}

void StmtBuilder::BuildIncDecStmt(ast::IncDecStmt* inc_dec_stmt, ASTContext& ast_ctx,
                                  IRContext& ir_ctx) {
  const ir::IntType* type = static_cast<const ir::IntType*>(
      type_builder_.BuildType(type_info_->TypeOf(inc_dec_stmt->x())));
  std::shared_ptr<ir::Computed> address =
      expr_builder_.BuildAddressOfExpr(inc_dec_stmt->x(), ast_ctx, ir_ctx);
  auto old_value = std::make_shared<ir::Computed>(type, ir_ctx.func()->next_computed_number());
  auto new_value = std::make_shared<ir::Computed>(type, ir_ctx.func()->next_computed_number());
  auto one = std::make_shared<ir::IntConstant>(common::Int(1).ConvertTo(type->int_type()));
  common::Int::BinaryOp op = [inc_dec_stmt]() {
    switch (inc_dec_stmt->tok()) {
      case tokens::kInc:
        return common::Int::BinaryOp::kAdd;
      case tokens::kDec:
        return common::Int::BinaryOp::kSub;
      default:
        common::fail("unexpected inc dec stmt token");
    }
  }();
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::LoadInstr>(old_value, address));
  ir_ctx.block()->instrs().push_back(
      std::make_unique<ir::IntBinaryInstr>(new_value, op, old_value, one));
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::StoreInstr>(address, new_value));
}

void StmtBuilder::BuildReturnStmt(ast::ReturnStmt* return_stmt, ASTContext& ast_ctx,
                                  IRContext& ir_ctx) {
  std::vector<std::shared_ptr<ir::Value>> results =
      expr_builder_.BuildValuesOfExprs(return_stmt->results(), ast_ctx, ir_ctx);

  BuildVarDeletionsForASTContextAndParents(&ast_ctx, ir_ctx);

  ir_ctx.block()->instrs().push_back(std::make_unique<ir::ReturnInstr>(results));
}

void StmtBuilder::BuildIfStmt(ast::IfStmt* if_stmt, ASTContext& ast_ctx, IRContext& ir_ctx) {
  if (if_stmt->init_stmt() != nullptr) {
    BuildStmt(if_stmt->init_stmt(), ast_ctx, ir_ctx);
  }
  std::shared_ptr<ir::Value> condition =
      expr_builder_.BuildValuesOfExpr(if_stmt->cond_expr(), ast_ctx, ir_ctx).front();

  ir::Block* start_block = ir_ctx.block();

  ir::Block* if_entry_block = ir_ctx.func()->AddBlock();
  IRContext if_ir_ctx = ir_ctx.ChildContextFor(if_entry_block);
  BuildBlockStmt(if_stmt->body(), ast_ctx, if_ir_ctx);
  ir::Block* if_exit_block = if_ir_ctx.block();

  bool has_else = if_stmt->else_stmt() != nullptr;
  ir::Block* else_entry_block = nullptr;
  ir::Block* else_exit_block = nullptr;
  std::optional<IRContext> else_ir_ctx;
  if (has_else) {
    else_entry_block = ir_ctx.func()->AddBlock();
    else_ir_ctx = ir_ctx.ChildContextFor(else_entry_block);
    BuildStmt(if_stmt->else_stmt(), ast_ctx, else_ir_ctx.value());
    else_exit_block = else_ir_ctx->block();
  }

  ir::Block* merge_block = ir_ctx.func()->AddBlock();
  ir_ctx.set_block(merge_block);

  ir::block_num_t destination_true = if_entry_block->number();
  ir::block_num_t destination_false =
      (has_else) ? else_entry_block->number() : merge_block->number();
  start_block->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(condition, destination_true, destination_false));
  if_exit_block->instrs().push_back(std::make_unique<ir::JumpInstr>(merge_block->number()));
  if (has_else) {
    else_exit_block->instrs().push_back(std::make_unique<ir::JumpInstr>(merge_block->number()));
  }

  ir_ctx.func()->AddControlFlow(start_block->number(), if_entry_block->number());
  ir_ctx.func()->AddControlFlow(if_exit_block->number(), merge_block->number());
  if (has_else) {
    ir_ctx.func()->AddControlFlow(start_block->number(), else_entry_block->number());
    ir_ctx.func()->AddControlFlow(else_exit_block->number(), merge_block->number());
  } else {
    ir_ctx.func()->AddControlFlow(start_block->number(), merge_block->number());
  }
}

void StmtBuilder::BuildExprSwitchStmt(ast::ExprSwitchStmt* expr_switch_stmt, ASTContext& ast_ctx,
                                      IRContext& ir_ctx) {
  // TODO: implement
}

void StmtBuilder::BuildTypeSwitchStmt(ast::TypeSwitchStmt* type_switch_stmt, ASTContext& ast_ctx,
                                      IRContext& ir_ctx) {
  // TODO: implement
}

void StmtBuilder::BuildForStmt(ast::ForStmt* for_stmt, ASTContext& ast_ctx, IRContext& ir_ctx) {
  if (for_stmt->init_stmt() != nullptr) {
    BuildStmt(for_stmt->init_stmt(), ast_ctx, ir_ctx);
  }

  ir::Block* start_block = ir_ctx.block();

  ir::Block* body_entry_block = ir_ctx.func()->AddBlock();
  IRContext body_ir_ctx = ir_ctx.ChildContextFor(body_entry_block);
  BuildBlockStmt(for_stmt->body(), ast_ctx, body_ir_ctx);
  if (for_stmt->post_stmt() != nullptr) {
    BuildStmt(for_stmt->post_stmt(), ast_ctx, body_ir_ctx);
  }
  ir::Block* body_exit_block = body_ir_ctx.block();

  ir::Block* continue_block = ir_ctx.func()->AddBlock();

  ir::Block* header_block = ir_ctx.func()->AddBlock();
  IRContext header_ir_ctx = ir_ctx.ChildContextFor(header_block);
  std::shared_ptr<ir::Value> condition =
      expr_builder_.BuildValuesOfExpr(for_stmt->cond_expr(), ast_ctx, header_ir_ctx).front();

  header_block->instrs().push_back(std::make_unique<ir::JumpCondInstr>(
      condition, body_entry_block->number(), continue_block->number()));
  start_block->instrs().push_back(std::make_unique<ir::JumpInstr>(header_block->number()));
  body_exit_block->instrs().push_back(std::make_unique<ir::JumpInstr>(header_block->number()));

  ir_ctx.func()->AddControlFlow(start_block->number(), header_block->number());
  ir_ctx.func()->AddControlFlow(header_block->number(), body_entry_block->number());
  ir_ctx.func()->AddControlFlow(header_block->number(), continue_block->number());
  ir_ctx.func()->AddControlFlow(body_exit_block->number(), header_block->number());

  ir_ctx.set_block(continue_block);
}

void StmtBuilder::BuildBranchStmt(ast::BranchStmt* branch_stmt, ASTContext& ast_ctx,
                                  IRContext& ir_ctx) {
  // TODO: implement
}

}  // namespace ir_builder
}  // namespace lang

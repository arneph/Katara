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

std::unique_ptr<ir::Program> IRBuilder::TranslateProgram(packages::Package* main_package,
                                                         types::Info* type_info) {
  auto prog = std::make_unique<ir::Program>();
  auto builder = std::unique_ptr<IRBuilder>(new IRBuilder(type_info, prog));

  for (auto [file_name, file] : main_package->ast_package()->files()) {
    builder->PrepareDeclsInFile(file);
  }
  for (auto [file_name, file] : main_package->ast_package()->files()) {
    builder->BuildDeclsInFile(file);
  }

  return prog;
}

IRBuilder::IRBuilder(types::Info* type_info, std::unique_ptr<ir::Program>& prog)
    : type_info_(type_info), program_(prog) {
  ir_string_type_ = static_cast<ir_ext::String*>(
      program_->type_table().AddType(std::make_unique<ir_ext::String>()));
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
  types::Func* types_func = static_cast<types::Func*>(type_info_->DefinitionOf(func_decl->name()));
  ir::Func* ir_func = program_->AddFunc();
  ir_func->set_name(func_decl->name()->name());
  funcs_.insert({types_func, ir_func});
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
  types::Func* types_func = static_cast<types::Func*>(type_info_->DefinitionOf(func_decl->name()));
  ir::Func* ir_func = funcs_.at(types_func);
  ir::Block* entry_block = ir_func->AddBlock();
  ir_func->set_entry_block_num(entry_block->number());
  Context ctx(ir_func);
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
    if (value_spec->values().empty()) {
      for (ast::Ident* name : value_spec->names()) {
        types::Variable* var = static_cast<types::Variable*>(type_info_->DefinitionOf(name));
        if (var == nullptr) {
          continue;
        }
        std::shared_ptr<ir::Value> default_value = DefaultValueForType(var->type());
        ctx.var_values().insert({var, default_value});
      }
    } else {
      std::vector<std::shared_ptr<ir::Value>> values = BuildExprs(value_spec->values(), ctx);
      for (size_t i = 0; i < value_spec->names().size() && i < values.size(); i++) {
        ast::Ident* name = value_spec->names().at(i);
        types::Variable* var = static_cast<types::Variable*>(type_info_->DefinitionOf(name));
        if (var == nullptr) {
          continue;
        }
        std::shared_ptr<ir::Value> value = values.at(i);
        ctx.var_values().insert({var, value});
      }
    }
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
  return {std::make_shared<ir::Constant>(program_->type_table().AtomicOfKind(ir::AtomicKind::kBool),
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
    case ast::NodeKind::kBasicLit:
      return {BuildBasicLit(static_cast<ast::BasicLit*>(expr))};
    case ast::NodeKind::kIdent:
      return {BuildIdent(static_cast<ast::Ident*>(expr), ctx)};
    default:
      throw "internal error: unexpected expr";
  }
}

std::shared_ptr<ir::Value> IRBuilder::BuildUnaryExpr(ast::UnaryExpr* expr, Context& ctx) {
  switch (expr->op()) {
    case tokens::kMul:
    case tokens::kRem:
    case tokens::kAnd:
      return BuildUnaryMemoryExpr(expr, ctx);
    case tokens::kAdd:
      return BuildExpr(expr->x(), ctx).front();
    case tokens::kSub:
      return BuildUnaryALExpr(expr, ir::UnaryALOperation::kNeg, ctx);
    case tokens::kXor:
    case tokens::kNot:
      return BuildUnaryALExpr(expr, ir::UnaryALOperation::kNot, ctx);
    default:
      throw "internal error: unexpected unary op";
  }
}

std::shared_ptr<ir::Value> IRBuilder::BuildUnaryALExpr(ast::UnaryExpr* expr,
                                                       ir::UnaryALOperation op, Context& ctx) {
  types::Basic* basic_type =
      static_cast<types::Basic*>(type_info_->ExprInfoOf(expr).value().type());
  ir::Type* ir_type = BasicToIRType(basic_type);
  std::shared_ptr<ir::Value> x = BuildExpr(expr->x(), ctx).front();
  x = ConvertToType(x, ir_type, ctx);
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_type, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir::UnaryALInstr>(op, result, x));
  return result;
}

std::shared_ptr<ir::Value> IRBuilder::BuildUnaryMemoryExpr(ast::UnaryExpr* expr, Context& ctx) {
  // TODO: implement
  return nullptr;
}

std::shared_ptr<ir::Value> IRBuilder::BuildBinaryExpr(ast::BinaryExpr* expr, Context& ctx) {
  switch (expr->op()) {
    case tokens::kAdd: {
      types::Basic* basic_type = static_cast<types::Basic*>(type_info_->ExprInfoOf(expr)->type());
      if (basic_type->kind() == types::Basic::kUntypedString ||
          basic_type->kind() == types::Basic::kString) {
        return BuildStringConcatExpr(expr, ctx);
      }
      return BuildBinaryALExpr(expr, ir::BinaryALOperation::kAdd, ctx);
    }
    case tokens::kSub:
      return BuildBinaryALExpr(expr, ir::BinaryALOperation::kSub, ctx);
    case tokens::kMul:
      return BuildBinaryALExpr(expr, ir::BinaryALOperation::kMul, ctx);
    case tokens::kQuo:
      return BuildBinaryALExpr(expr, ir::BinaryALOperation::kDiv, ctx);
    case tokens::kRem:
      return BuildBinaryALExpr(expr, ir::BinaryALOperation::kRem, ctx);
    case tokens::kAnd:
      return BuildBinaryALExpr(expr, ir::BinaryALOperation::kAnd, ctx);
    case tokens::kOr:
      return BuildBinaryALExpr(expr, ir::BinaryALOperation::kOr, ctx);
    case tokens::kXor:
      return BuildBinaryALExpr(expr, ir::BinaryALOperation::kXor, ctx);
    case tokens::kAndNot:
      return BuildBinaryALExpr(expr, ir::BinaryALOperation::kAndNot, ctx);
    case tokens::kShl:
      return BuildBinaryShiftExpr(expr, ir::ShiftOperation::kShl, ctx);
    case tokens::kShr:
      return BuildBinaryShiftExpr(expr, ir::ShiftOperation::kShr, ctx);
    case tokens::kLAnd:
    case tokens::kLOr:
      return BuildBinaryLogicExpr(expr, ctx);
    default:
      throw "internal error: unexpected binary op";
  }
}

std::shared_ptr<ir::Value> IRBuilder::BuildStringConcatExpr(ast::BinaryExpr* expr, Context& ctx) {
  std::shared_ptr<ir::Value> x = BuildExpr(expr->x(), ctx).front();
  std::shared_ptr<ir::Value> y = BuildExpr(expr->x(), ctx).front();
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_string_type_, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir_ext::StringConcatInstr>(
      result, std::vector<std::shared_ptr<ir::Value>>{x, y}));
  return result;
}

std::shared_ptr<ir::Value> IRBuilder::BuildBinaryALExpr(ast::BinaryExpr* expr,
                                                        ir::BinaryALOperation op, Context& ctx) {
  types::Basic* basic_type =
      static_cast<types::Basic*>(type_info_->ExprInfoOf(expr).value().type());
  ir::Type* ir_type = BasicToIRType(basic_type);
  std::shared_ptr<ir::Value> x = BuildExpr(expr->x(), ctx).front();
  x = ConvertToType(x, ir_type, ctx);
  std::shared_ptr<ir::Value> y = BuildExpr(expr->x(), ctx).front();
  y = ConvertToType(y, ir_type, ctx);
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_type, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir::BinaryALInstr>(op, result, x, y));
  return result;
}

std::shared_ptr<ir::Value> IRBuilder::BuildBinaryShiftExpr(ast::BinaryExpr* expr,
                                                           ir::ShiftOperation op, Context& ctx) {
  types::Basic* basic_type =
      static_cast<types::Basic*>(type_info_->ExprInfoOf(expr).value().type());
  ir::Type* ir_type = BasicToIRType(basic_type);
  std::shared_ptr<ir::Value> x = BuildExpr(expr->x(), ctx).front();
  x = ConvertToType(x, ir_type, ctx);
  std::shared_ptr<ir::Value> y = BuildExpr(expr->x(), ctx).front();
  y = ConvertToType(y, program_->type_table().AtomicOfKind(ir::AtomicKind::kU64), ctx);
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_type, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir::ShiftInstr>(op, result, x, y));
  return result;
}

std::shared_ptr<ir::Value> IRBuilder::BuildBinaryLogicExpr(ast::BinaryExpr* expr, Context& ctx) {
  std::shared_ptr<ir::Value> x = BuildExpr(expr->x(), ctx).front();
  ir::Block* x_exit_block = ctx.block();

  ir::Block* y_entry_block = ctx.func()->AddBlock();
  Context y_ctx = ctx.SubContextForBlock(y_entry_block);
  std::shared_ptr<ir::Value> y = BuildExpr(expr->x(), y_ctx).front();
  ir::Block* y_exit_block = y_ctx.block();

  ir::Block* merge_block = ctx.func()->AddBlock();
  ctx.set_block(merge_block);

  ir::block_num_t destination_true, destination_false;
  ir::Atomic* atomic_bool = program_->type_table().AtomicOfKind(ir::AtomicKind::kBool);
  std::shared_ptr<ir::Constant> short_circuit_value;
  switch (expr->op()) {
    case tokens::kLAnd:
      destination_true = y_entry_block->number();
      destination_false = merge_block->number();
      short_circuit_value = std::make_shared<ir::Constant>(atomic_bool, 0);
      break;
    case tokens::kLOr:
      destination_true = merge_block->number();
      destination_false = y_entry_block->number();
      short_circuit_value = std::make_shared<ir::Constant>(atomic_bool, 1);
      break;
    default:
      throw "internal error: unexpected logic op";
  }

  x_exit_block->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(x, destination_true, destination_false));
  y_exit_block->instrs().push_back(std::make_unique<ir::JumpInstr>(merge_block->number()));

  auto result = std::make_shared<ir::Computed>(atomic_bool, ctx.func()->next_computed_number());
  auto inherited_short_circuit_value =
      std::make_shared<ir::InheritedValue>(short_circuit_value, x_exit_block->number());
  auto inherited_y = std::make_shared<ir::InheritedValue>(y, y_exit_block->number());
  merge_block->instrs().push_back(
      std::make_unique<ir::PhiInstr>(result, std::vector<std::shared_ptr<ir::InheritedValue>>{
                                                 inherited_short_circuit_value, inherited_y}));

  ctx.func()->AddControlFlow(x_exit_block->number(), y_entry_block->number());
  ctx.func()->AddControlFlow(x_exit_block->number(), merge_block->number());
  ctx.func()->AddControlFlow(y_exit_block->number(), merge_block->number());

  return result;
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

std::shared_ptr<ir::Value> IRBuilder::BuildBasicLit(ast::BasicLit* basic_lit) {
  types::ExprInfo lit_info = type_info_->ExprInfoOf(basic_lit).value();
  types::Basic* basic_type = static_cast<types::Basic*>(lit_info.type());
  return ConstantToIRValue(basic_type, lit_info.constant_value());
}

std::shared_ptr<ir::Value> IRBuilder::BuildIdent(ast::Ident* ident, Context& ctx) {
  types::Object* object = type_info_->uses().at(ident);
  types::ExprInfo ident_info = type_info_->ExprInfoOf(ident).value();
  switch (object->object_kind()) {
    case types::ObjectKind::kConstant: {
      types::Constant* constant = static_cast<types::Constant*>(object);
      types::Basic* type = static_cast<types::Basic*>(constant->type());
      return ConstantToIRValue(type, ident_info.constant_value());
    }
    case types::ObjectKind::kVariable: {
      types::Variable* variable = static_cast<types::Variable*>(object);
      return ctx.var_values().at(variable);
    }
    case types::ObjectKind::kFunc: {
      types::Func* types_func = static_cast<types::Func*>(object);
      ir::Func* ir_func = funcs_.at(types_func);
      ir::Atomic* atomic_type = program_->type_table().AtomicOfKind(ir::AtomicKind::kFunc);
      return std::make_shared<ir::Constant>(atomic_type, ir_func->number());
    }
    case types::ObjectKind::kNil: {
      ir::Atomic* atomic_type = program_->type_table().AtomicOfKind(ir::AtomicKind::kPtr);
      return std::make_shared<ir::Constant>(atomic_type, 0);
    }
    default:
      throw "internal error: unexpected object kind";
  }
}

std::shared_ptr<ir::Value> IRBuilder::ConvertToType(std::shared_ptr<ir::Value> value,
                                                    ir::Type* desired_type, Context& ctx) {
  if (value->type() == desired_type) {
    return value;
  } else if (value->type()->type_kind() == ir::TypeKind::kAtomic &&
             desired_type->type_kind() == ir::TypeKind::kAtomic) {
    std::shared_ptr<ir::Computed> result =
        std::make_shared<ir::Computed>(desired_type, ctx.func()->next_computed_number());
    ctx.block()->instrs().push_back(std::make_unique<ir::Conversion>(result, value));
    return result;
  } else {
    throw "internal error: unexpected conversion";
  }
}

std::shared_ptr<ir::Value> IRBuilder::DefaultValueForType(types::Type* lang_type) {
  switch (lang_type->type_kind()) {
    case types::TypeKind::kBasic: {
      ir::Type* ir_type = BasicToIRType(static_cast<types::Basic*>(lang_type));
      switch (ir_type->type_kind()) {
        case ir::TypeKind::kAtomic:
          return std::make_shared<ir::Constant>(static_cast<ir::Atomic*>(ir_type), 0);
        case ir::TypeKind::kLangString:
          return std::make_shared<ir_ext::StringConstant>(ir_string_type_, "");
        default:
          throw "internal error: unexpected ir type for basic type";
      }
    }
    default:
      return nullptr;
      // TODO: implement more types
      // throw "internal error: unexpected lang type";
  }
}

std::shared_ptr<ir::Value> IRBuilder::ConstantToIRValue(types::Basic* basic,
                                                        constants::Value constant) const {
  switch (basic->kind()) {
    case types::Basic::kBool:
    case types::Basic::kUntypedBool: {
      ir::Atomic* atomic_type = program_->type_table().AtomicOfKind(ir::AtomicKind::kBool);
      int64_t value = std::get<bool>(constant.value());
      return std::make_shared<ir::Constant>(atomic_type, value);
    }
    case types::Basic::kInt8: {
      ir::Atomic* atomic_type = program_->type_table().AtomicOfKind(ir::AtomicKind::kI8);
      int64_t value = std::get<int8_t>(constant.value());
      return std::make_shared<ir::Constant>(atomic_type, value);
    }
    case types::Basic::kInt16: {
      ir::Atomic* atomic_type = program_->type_table().AtomicOfKind(ir::AtomicKind::kI16);
      int64_t value = std::get<int16_t>(constant.value());
      return std::make_shared<ir::Constant>(atomic_type, value);
    }
    case types::Basic::kInt32:
    case types::Basic::kUntypedRune: {
      ir::Atomic* atomic_type = program_->type_table().AtomicOfKind(ir::AtomicKind::kI32);
      int64_t value = std::get<int32_t>(constant.value());
      return std::make_shared<ir::Constant>(atomic_type, value);
    }
    case types::Basic::kInt:
    case types::Basic::kInt64:
    case types::Basic::kUntypedInt: {
      ir::Atomic* atomic_type = program_->type_table().AtomicOfKind(ir::AtomicKind::kI64);
      int64_t value = std::get<int64_t>(constant.value());
      return std::make_shared<ir::Constant>(atomic_type, value);
    }
    case types::Basic::kUint8: {
      ir::Atomic* atomic_type = program_->type_table().AtomicOfKind(ir::AtomicKind::kU8);
      int64_t value = std::get<uint8_t>(constant.value());
      return std::make_shared<ir::Constant>(atomic_type, value);
    }
    case types::Basic::kUint16: {
      ir::Atomic* atomic_type = program_->type_table().AtomicOfKind(ir::AtomicKind::kU16);
      int64_t value = std::get<uint16_t>(constant.value());
      return std::make_shared<ir::Constant>(atomic_type, value);
    }
    case types::Basic::kUint32: {
      ir::Atomic* atomic_type = program_->type_table().AtomicOfKind(ir::AtomicKind::kU32);
      int64_t value = std::get<uint32_t>(constant.value());
      return std::make_shared<ir::Constant>(atomic_type, value);
    }
    case types::Basic::kUint:
    case types::Basic::kUint64: {
      ir::Atomic* atomic_type = program_->type_table().AtomicOfKind(ir::AtomicKind::kU64);
      int64_t value = std::get<uint64_t>(constant.value());
      return std::make_shared<ir::Constant>(atomic_type, value);
    }
    case types::Basic::kString:
    case types::Basic::kUntypedString: {
      std::string value = std::get<std::string>(constant.value());
      return std::make_shared<ir_ext::StringConstant>(ir_string_type_, value);
    }
    default:
      throw "internal error: unexpected basic literal type";
  }
}

ir::Type* IRBuilder::BasicToIRType(types::Basic* basic) const {
  switch (basic->kind()) {
    case types::Basic::kBool:
    case types::Basic::kUntypedBool:
      return program_->type_table().AtomicOfKind(ir::AtomicKind::kBool);
    case types::Basic::kInt8:
      return program_->type_table().AtomicOfKind(ir::AtomicKind::kI8);
    case types::Basic::kInt16:
      return program_->type_table().AtomicOfKind(ir::AtomicKind::kI16);
    case types::Basic::kInt32:
    case types::Basic::kUntypedRune:
      return program_->type_table().AtomicOfKind(ir::AtomicKind::kI32);
    case types::Basic::kInt:
    case types::Basic::kInt64:
    case types::Basic::kUntypedInt:
      return program_->type_table().AtomicOfKind(ir::AtomicKind::kI64);
    case types::Basic::kUint8:
      return program_->type_table().AtomicOfKind(ir::AtomicKind::kU8);
    case types::Basic::kUint16:
      return program_->type_table().AtomicOfKind(ir::AtomicKind::kU16);
    case types::Basic::kUint32:
      return program_->type_table().AtomicOfKind(ir::AtomicKind::kU32);
    case types::Basic::kUint:
    case types::Basic::kUint64:
      return program_->type_table().AtomicOfKind(ir::AtomicKind::kU64);
    case types::Basic::kString:
    case types::Basic::kUntypedString:
      return ir_string_type_;
    case types::Basic::kUntypedNil:
      return program_->type_table().AtomicOfKind(ir::AtomicKind::kPtr);
    default:
      throw "internal error: unexpected basic literal type";
  }
}

}  // namespace ir_builder
}  // namespace lang

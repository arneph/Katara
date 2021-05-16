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
  ir_ref_count_ptr_type_ = static_cast<ir_ext::RefCountPointer*>(
      program_->type_table().AddType(std::make_unique<ir_ext::RefCountPointer>()));
  ir_string_type_ = static_cast<ir_ext::String*>(
      program_->type_table().AddType(std::make_unique<ir_ext::String>()));
  ir_empty_struct_ = static_cast<ir_ext::Struct*>(program_->type_table().AddType(
      std::make_unique<ir_ext::Struct>(std::vector<std::pair<std::string, ir::Type*>>{})));
  ir_empty_interface_ = static_cast<ir_ext::Interface*>(program_->type_table().AddType(
      std::make_unique<ir_ext::Interface>(std::vector<std::string>{})));
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
  BuildPrologForFunc(types_func, ctx);
  BuildBlockStmt(func_decl->body(), ctx);
  if (ctx.block()->instrs().empty() ||
      ctx.block()->instrs().back()->instr_kind() != ir::InstrKind::kReturn) {
    BuildEpilogForFunc(ctx);
  }
}

void IRBuilder::BuildPrologForFunc(types::Func* types_func, Context& ctx) {
  types::Signature* signature = static_cast<types::Signature*>(types_func->type());
  for (types::Variable* param : signature->parameters()->variables()) {
    AddVarMalloc(param, ctx);
  }
  if (signature->results() != nullptr) {
    for (types::Variable* result : signature->results()->variables()) {
      if (result->name().empty()) {
        continue;
      }
      AddVarMalloc(result, ctx);
    }
  }
  // TODO: handle args and results
}

void IRBuilder::BuildEpilogForFunc(Context& ctx) {
  for (const Context* c = &ctx; c != nullptr; c = c->parent_ctx()) {
    for (auto [var, address] : c->var_addresses()) {
      AddVarRelease(var, ctx);
    }
  }
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
    for (ast::Ident* name : value_spec->names()) {
      types::Variable* var = static_cast<types::Variable*>(type_info_->DefinitionOf(name));
      if (var == nullptr) {
        continue;
      }
      AddVarMalloc(var, ctx);
    }

    if (value_spec->values().empty()) {
      for (ast::Ident* name : value_spec->names()) {
        types::Variable* var = static_cast<types::Variable*>(type_info_->DefinitionOf(name));
        if (var == nullptr) {
          continue;
        }
        std::shared_ptr<ir::Value> address = ctx.LookupAddressOfVar(var);
        std::shared_ptr<ir::Value> default_value = DefaultIRValueForType(var->type());
        ctx.block()->instrs().push_back(std::make_unique<ir::StoreInstr>(address, default_value));
      }
    } else {
      std::vector<std::shared_ptr<ir::Value>> values =
          BuildValuesOfExprs(value_spec->values(), ctx);
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

void IRBuilder::BuildAssignStmt(ast::AssignStmt* assign_stmt, Context& ctx) {
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
      AddVarMalloc(var, ctx);
    }
  }

  std::vector<std::shared_ptr<ir::Value>> lhs_addresses =
      BuildAddressesOfExprs(assign_stmt->lhs(), ctx);
  std::vector<std::shared_ptr<ir::Value>> rhs_values = BuildValuesOfExprs(assign_stmt->rhs(), ctx);

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

void IRBuilder::BuildExprStmt(ast::ExprStmt* expr_stmt, Context& ctx) {
  BuildValuesOfExpr(expr_stmt->x(), ctx);
}

void IRBuilder::BuildIncDecStmt(ast::IncDecStmt* inc_dec_stmt, Context& ctx) {
  // TODO: implement
}

void IRBuilder::BuildReturnStmt(ast::ReturnStmt* return_stmt, Context& ctx) {
  std::vector<std::shared_ptr<ir::Value>> results = BuildValuesOfExprs(return_stmt->results(), ctx);
}

void IRBuilder::BuildIfStmt(ast::IfStmt* if_stmt, Context& ctx) {
  if (if_stmt->init_stmt() != nullptr) {
    BuildStmt(if_stmt->init_stmt(), ctx);
  }
  std::shared_ptr<ir::Value> condition = BuildValuesOfExpr(if_stmt->cond_expr(), ctx).front();

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
  std::shared_ptr<ir::Value> condition =
      BuildValuesOfExpr(for_stmt->cond_expr(), header_ctx).front();

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

void IRBuilder::AddVarMalloc(types::Variable* var, Context& ctx) {
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_ref_count_ptr_type_, ctx.func()->next_computed_number());
  ir::Type* type = ToIRType(var->type());
  ctx.block()->instrs().push_back(std::make_unique<ir_ext::RefCountMallocInstr>(result, type));
  ctx.AddAddressOfVar(var, result);
}

void IRBuilder::AddVarRetain(types::Variable* var, Context& ctx) {
  std::shared_ptr<ir::Value> address = ctx.LookupAddressOfVar(var);
  ctx.block()->instrs().push_back(
      std::make_unique<ir_ext::RefCountUpdateInstr>(ir_ext::RefCountUpdate::kInc, address));
}

void IRBuilder::AddVarRelease(types::Variable* var, Context& ctx) {
  std::shared_ptr<ir::Value> address = ctx.LookupAddressOfVar(var);
  ctx.block()->instrs().push_back(
      std::make_unique<ir_ext::RefCountUpdateInstr>(ir_ext::RefCountUpdate::kDec, address));
}

std::vector<std::shared_ptr<ir::Value>> IRBuilder::BuildAddressesOfExprs(
    std::vector<ast::Expr*> exprs, Context& ctx) {
  std::vector<std::shared_ptr<ir::Value>> addresses;
  for (auto expr : exprs) {
    addresses.push_back(BuildAddressOfExpr(expr, ctx));
  }
  return addresses;
}

std::shared_ptr<ir::Value> IRBuilder::BuildAddressOfExpr(ast::Expr* expr, Context& ctx) {
  switch (expr->node_kind()) {
    case ast::NodeKind::kUnaryExpr:
      return BuildAddressOfUnaryMemoryExpr(static_cast<ast::UnaryExpr*>(expr), ctx);
    case ast::NodeKind::kSelectionExpr:
      return BuildAddressOfStructFieldSelectionExpr(static_cast<ast::SelectionExpr*>(expr), ctx);
    case ast::NodeKind::kIndexExpr:
      return BuildAddressOfIndexExpr(static_cast<ast::IndexExpr*>(expr), ctx);
    case ast::NodeKind::kIdent:
      return BuildAddressOfIdent(static_cast<ast::Ident*>(expr), ctx);
    default:
      throw "internal error: unexpected addressable expr";
  }
}

std::vector<std::shared_ptr<ir::Value>> IRBuilder::BuildValuesOfExprs(std::vector<ast::Expr*> exprs,
                                                                      Context& ctx) {
  std::vector<std::shared_ptr<ir::Value>> values;
  for (auto expr : exprs) {
    std::vector<std::shared_ptr<ir::Value>> expr_values = BuildValuesOfExpr(expr, ctx);
    if (!expr_values.empty()) {
      values.push_back(expr_values.front());
    }
  }
  return values;
}

std::vector<std::shared_ptr<ir::Value>> IRBuilder::BuildValuesOfExpr(ast::Expr* expr,
                                                                     Context& ctx) {
  //  return
  //  {std::make_shared<ir::Constant>(program_->type_table().AtomicOfKind(ir::AtomicKind::kBool),
  //                                         1)};  // TODO: remove
  switch (expr->node_kind()) {
    case ast::NodeKind::kUnaryExpr:
      return {BuildValueOfUnaryExpr(static_cast<ast::UnaryExpr*>(expr), ctx)};
    case ast::NodeKind::kBinaryExpr:
      return {BuildValueOfBinaryExpr(static_cast<ast::BinaryExpr*>(expr), ctx)};
    case ast::NodeKind::kCompareExpr:
      return {BuildValueOfCompareExpr(static_cast<ast::CompareExpr*>(expr), ctx)};
    case ast::NodeKind::kParenExpr:
      return BuildValuesOfExpr(static_cast<ast::ParenExpr*>(expr)->x(), ctx);
    case ast::NodeKind::kSelectionExpr:
      return BuildValuesOfSelectionExpr(static_cast<ast::SelectionExpr*>(expr), ctx);
    case ast::NodeKind::kTypeAssertExpr:
      return BuildValuesOfTypeAssertExpr(static_cast<ast::TypeAssertExpr*>(expr), ctx);
    case ast::NodeKind::kIndexExpr:
      return {BuildValueOfIndexExpr(static_cast<ast::IndexExpr*>(expr), ctx)};
    case ast::NodeKind::kCallExpr:
      return BuildValuesOfCallExpr(static_cast<ast::CallExpr*>(expr), ctx);
    case ast::NodeKind::kFuncLit:
      return {BuildValueOfFuncLit(static_cast<ast::FuncLit*>(expr), ctx)};
    case ast::NodeKind::kCompositeLit:
      return {BuildValueOfCompositeLit(static_cast<ast::CompositeLit*>(expr), ctx)};
    case ast::NodeKind::kBasicLit:
      return {BuildValueOfBasicLit(static_cast<ast::BasicLit*>(expr))};
    case ast::NodeKind::kIdent:
      return {BuildValueOfIdent(static_cast<ast::Ident*>(expr), ctx)};
    default:
      throw "internal error: unexpected expr";
  }
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfUnaryExpr(ast::UnaryExpr* expr, Context& ctx) {
  switch (expr->op()) {
    case tokens::kMul:
    case tokens::kRem:
    case tokens::kAnd:
      return BuildValueOfUnaryMemoryExpr(expr, ctx);
    case tokens::kAdd:
      return BuildValuesOfExpr(expr->x(), ctx).front();
    case tokens::kSub:
      return BuildValueOfUnaryALExpr(expr, ir::UnaryALOperation::kNeg, ctx);
    case tokens::kXor:
    case tokens::kNot:
      return BuildValueOfUnaryALExpr(expr, ir::UnaryALOperation::kNot, ctx);
    default:
      throw "internal error: unexpected unary op";
  }
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfUnaryALExpr(ast::UnaryExpr* expr,
                                                              ir::UnaryALOperation op,
                                                              Context& ctx) {
  types::Basic* basic_type =
      static_cast<types::Basic*>(type_info_->ExprInfoOf(expr).value().type());
  ir::Type* ir_type = ToIRType(basic_type);
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ctx).front();
  x = BuildValueOfConversion(x, ir_type, ctx);
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_type, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir::UnaryALInstr>(op, result, x));
  return result;
}

std::shared_ptr<ir::Value> IRBuilder::BuildAddressOfUnaryMemoryExpr(ast::UnaryExpr* expr,
                                                                    Context& ctx) {
  if (expr->op() == tokens::kMul || expr->op() == tokens::kRem) {
    return BuildValuesOfExpr(expr->x(), ctx).front();
  } else {
    throw "internal error: unexpected unary memory expr";
  }
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfUnaryMemoryExpr(ast::UnaryExpr* expr,
                                                                  Context& ctx) {
  ast::Expr* x = expr->x();
  if (expr->op() == tokens::kAnd) {
    if (x->node_kind() == ast::NodeKind::kCompositeLit) {
      types::Type* types_struct_type = type_info_->ExprInfoOf(x)->type();
      ir::Type* ir_struct_type = ToIRType(types_struct_type);
      std::shared_ptr<ir::Value> struct_value =
          BuildValueOfCompositeLit(static_cast<ast::CompositeLit*>(x), ctx);
      std::shared_ptr<ir::Computed> struct_address = std::make_shared<ir::Computed>(
          ir_ref_count_ptr_type_, ctx.func()->next_computed_number());
      ctx.block()->instrs().push_back(
          std::make_unique<ir_ext::RefCountMallocInstr>(struct_address, ir_struct_type));
      ctx.block()->instrs().push_back(
          std::make_unique<ir::StoreInstr>(struct_address, struct_value));
      return struct_address;
    } else {
      std::shared_ptr<ir::Value> address = BuildAddressOfExpr(x, ctx);
      if (address->type()->type_kind() == ir::TypeKind::kLangRefCountPointer) {
        ctx.block()->instrs().push_back(
            std::make_unique<ir_ext::RefCountUpdateInstr>(ir_ext::RefCountUpdate::kInc, address));
        // TODO: decrease ref count when variable exceeds scope
      }
      return address;
    }

  } else if (expr->op() == tokens::kMul || expr->op() == tokens::kRem) {
    std::shared_ptr<ir::Value> address = BuildAddressOfExpr(x, ctx);
    types::Type* types_value_type = type_info_->ExprInfoOf(x)->type();
    ir::Type* ir_value_type = ToIRType(types_value_type);
    std::shared_ptr<ir::Computed> value =
        std::make_shared<ir::Computed>(ir_value_type, ctx.func()->next_computed_number());
    ctx.block()->instrs().push_back(std::make_unique<ir::LoadInstr>(value, address));
    return value;
  } else {
    throw "internal error: unexpected unary memory expr";
  }
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfBinaryExpr(ast::BinaryExpr* expr, Context& ctx) {
  switch (expr->op()) {
    case tokens::kAdd: {
      types::Basic* basic_type = static_cast<types::Basic*>(type_info_->ExprInfoOf(expr)->type());
      if (basic_type->kind() == types::Basic::kUntypedString ||
          basic_type->kind() == types::Basic::kString) {
        return BuildValueOfStringConcatExpr(expr, ctx);
      }
      return BuildValueOfBinaryALExpr(expr, ir::BinaryALOperation::kAdd, ctx);
    }
    case tokens::kSub:
      return BuildValueOfBinaryALExpr(expr, ir::BinaryALOperation::kSub, ctx);
    case tokens::kMul:
      return BuildValueOfBinaryALExpr(expr, ir::BinaryALOperation::kMul, ctx);
    case tokens::kQuo:
      return BuildValueOfBinaryALExpr(expr, ir::BinaryALOperation::kDiv, ctx);
    case tokens::kRem:
      return BuildValueOfBinaryALExpr(expr, ir::BinaryALOperation::kRem, ctx);
    case tokens::kAnd:
      return BuildValueOfBinaryALExpr(expr, ir::BinaryALOperation::kAnd, ctx);
    case tokens::kOr:
      return BuildValueOfBinaryALExpr(expr, ir::BinaryALOperation::kOr, ctx);
    case tokens::kXor:
      return BuildValueOfBinaryALExpr(expr, ir::BinaryALOperation::kXor, ctx);
    case tokens::kAndNot:
      return BuildValueOfBinaryALExpr(expr, ir::BinaryALOperation::kAndNot, ctx);
    case tokens::kShl:
      return BuildValueOfBinaryShiftExpr(expr, ir::ShiftOperation::kShl, ctx);
    case tokens::kShr:
      return BuildValueOfBinaryShiftExpr(expr, ir::ShiftOperation::kShr, ctx);
    case tokens::kLAnd:
    case tokens::kLOr:
      return BuildValueOfBinaryLogicExpr(expr, ctx);
    default:
      throw "internal error: unexpected binary op";
  }
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfStringConcatExpr(ast::BinaryExpr* expr,
                                                                   Context& ctx) {
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ctx).front();
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(expr->x(), ctx).front();
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_string_type_, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir_ext::StringConcatInstr>(
      result, std::vector<std::shared_ptr<ir::Value>>{x, y}));
  return result;
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfBinaryALExpr(ast::BinaryExpr* expr,
                                                               ir::BinaryALOperation op,
                                                               Context& ctx) {
  types::Basic* basic_type =
      static_cast<types::Basic*>(type_info_->ExprInfoOf(expr).value().type());
  ir::Type* ir_type = ToIRType(basic_type);
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ctx).front();
  x = BuildValueOfConversion(x, ir_type, ctx);
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(expr->x(), ctx).front();
  y = BuildValueOfConversion(y, ir_type, ctx);
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_type, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir::BinaryALInstr>(op, result, x, y));
  return result;
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfBinaryShiftExpr(ast::BinaryExpr* expr,
                                                                  ir::ShiftOperation op,
                                                                  Context& ctx) {
  types::Basic* basic_type =
      static_cast<types::Basic*>(type_info_->ExprInfoOf(expr).value().type());
  ir::Type* ir_type = ToIRType(basic_type);
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ctx).front();
  x = BuildValueOfConversion(x, ir_type, ctx);
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(expr->x(), ctx).front();
  y = BuildValueOfConversion(y, program_->type_table().AtomicOfKind(ir::AtomicKind::kU64), ctx);
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_type, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir::ShiftInstr>(op, result, x, y));
  return result;
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfBinaryLogicExpr(ast::BinaryExpr* expr,
                                                                  Context& ctx) {
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ctx).front();
  ir::Block* x_exit_block = ctx.block();

  ir::Block* y_entry_block = ctx.func()->AddBlock();
  Context y_ctx = ctx.SubContextForBlock(y_entry_block);
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(expr->x(), y_ctx).front();
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

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfCompareExpr(ast::CompareExpr* expr,
                                                              Context& ctx) {
  if (expr->compare_ops().size() == 1) {
    return {BuildValueOfSingleCompareExpr(expr, ctx)};
  } else {
    return {BuildValueOfMultipleCompareExpr(expr, ctx)};
  }
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfSingleCompareExpr(ast::CompareExpr* expr,
                                                                    Context& ctx) {
  ast::Expr* x_expr = expr->operands().front();
  types::Type* x_type = type_info_->ExprInfoOf(x_expr)->type();
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(x_expr, ctx).front();

  ast::Expr* y_expr = expr->operands().back();
  types::Type* y_type = type_info_->ExprInfoOf(y_expr)->type();
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(y_expr, ctx).front();

  return BuildValueOfComparison(expr->compare_ops().front(), x, x_type, y, y_type, ctx);
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfMultipleCompareExpr(ast::CompareExpr* expr,
                                                                      Context& ctx) {
  ast::Expr* x_expr = expr->operands().front();
  types::Type* x_type = type_info_->ExprInfoOf(x_expr)->type();
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(x_expr, ctx).front();

  tokens::Token op = expr->compare_ops().front();
  ast::Expr* y_expr = expr->operands().at(1);
  types::Type* y_type = type_info_->ExprInfoOf(y_expr)->type();
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(y_expr, ctx).front();

  std::shared_ptr<ir::Value> partial_result = BuildValueOfComparison(op, x, x_type, y, y_type, ctx);

  ir::Block* prior_block = ctx.block();
  ir::Block* merge_block = ctx.func()->AddBlock();

  ir::Atomic* atomic_bool = program_->type_table().AtomicOfKind(ir::AtomicKind::kBool);
  std::shared_ptr<ir::Constant> false_value = std::make_shared<ir::Constant>(atomic_bool, 0);
  std::vector<std::shared_ptr<ir::InheritedValue>> merge_values;

  for (size_t i = 1; i < expr->compare_ops().size(); i++) {
    ir::Block* start_block = ctx.func()->AddBlock();
    ctx.set_block(start_block);

    prior_block->instrs().push_back(std::make_unique<ir::JumpCondInstr>(
        partial_result, start_block->number(), merge_block->number()));
    ctx.func()->AddControlFlow(prior_block->number(), start_block->number());
    ctx.func()->AddControlFlow(prior_block->number(), merge_block->number());
    merge_values.push_back(
        std::make_shared<ir::InheritedValue>(false_value, prior_block->number()));

    x_expr = y_expr;
    x_type = y_type;
    x = y;

    op = expr->compare_ops().at(i);
    y_expr = expr->operands().at(i + 1);
    y_type = type_info_->ExprInfoOf(y_expr)->type();
    y = BuildValuesOfExpr(y_expr, ctx).front();

    partial_result = BuildValueOfComparison(op, x, x_type, y, y_type, ctx);
    prior_block = ctx.block();

    if (i == expr->compare_ops().size() - 1) {
      prior_block->instrs().push_back(std::make_unique<ir::JumpInstr>(merge_block->number()));
      ctx.func()->AddControlFlow(prior_block->number(), merge_block->number());
      merge_values.push_back(
          std::make_shared<ir::InheritedValue>(partial_result, prior_block->number()));
    }
  }

  ctx.set_block(merge_block);

  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(atomic_bool, ctx.func()->next_computed_number());
  merge_block->instrs().push_back(std::make_unique<ir::PhiInstr>(result, merge_values));

  return result;
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfComparison(tokens::Token op,
                                                             std::shared_ptr<ir::Value> x,
                                                             types::Type* x_type,
                                                             std::shared_ptr<ir::Value> y,
                                                             types::Type* y_type, Context& ctx) {
  // TODO: implement
  return {std::make_shared<ir::Constant>(program_->type_table().AtomicOfKind(ir::AtomicKind::kBool),
                                         1)};
}

std::vector<std::shared_ptr<ir::Value>> IRBuilder::BuildValuesOfSelectionExpr(
    ast::SelectionExpr* expr, Context& ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Value> IRBuilder::BuildAddressOfStructFieldSelectionExpr(
    ast::SelectionExpr* expr, Context& ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfStructFieldSelectionExpr(ast::SelectionExpr* expr,
                                                                           Context& ctx) {
  // TODO: implement
  return {};
}

std::vector<std::shared_ptr<ir::Value>> IRBuilder::BuildValuesOfTypeAssertExpr(
    ast::TypeAssertExpr* expr, Context& ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Value> IRBuilder::BuildAddressOfIndexExpr(ast::IndexExpr* expr, Context& ctx) {
  // TODO: implement (array, slice)
  std::shared_ptr<ir::Computed> address =
      std::make_shared<ir::Computed>(ir_ref_count_ptr_type_, ctx.func()->next_computed_number());
  return address;
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfIndexExpr(ast::IndexExpr* expr, Context& ctx) {
  ast::Expr* accessed_expr = expr->accessed();
  ast::Expr* index_expr = expr->index();
  types::InfoBuilder info_builder = type_info_->builder();
  types::Type* types_accessed_type = type_info_->ExprInfoOf(accessed_expr)->type();
  types::Type* types_accessed_underlying_type = types::UnderlyingOf(types_accessed_type, info_builder);
  if (types_accessed_underlying_type->type_kind() == types::TypeKind::kBasic) {
    // Note: strings are the only basic type that can be indexed
    ir::Type* rune_type = program_->type_table().AtomicOfKind(ir::AtomicKind::kI32);
    std::shared_ptr<ir::Value> string = BuildValuesOfExpr(accessed_expr, ctx).front();
    std::shared_ptr<ir::Value> index = BuildValuesOfExpr(index_expr, ctx).front();
    std::shared_ptr<ir::Computed> value =
        std::make_shared<ir::Computed>(rune_type, ctx.func()->next_computed_number());
    ctx.block()->instrs().push_back(
        std::make_unique<ir_ext::StringIndexInstr>(value, string, index));
    return value;

  } else if (types_accessed_underlying_type->is_container()) {
    types::Type* types_element_type = type_info_->ExprInfoOf(expr)->type();
    ir::Type* ir_element_type = ToIRType(types_element_type);
    std::shared_ptr<ir::Computed> value =
        std::make_shared<ir::Computed>(ir_element_type, ctx.func()->next_computed_number());
    // TODO: actually add load instr
    return value;
  } else {
    throw "internal error: unexpected accessed value in index expr";
  }
}

std::vector<std::shared_ptr<ir::Value>> IRBuilder::BuildValuesOfCallExpr(ast::CallExpr* expr,
                                                                         Context& ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Constant> IRBuilder::BuildValueOfFuncLit(ast::FuncLit* expr, Context& ctx) {
  // TODO: implement
  return nullptr;
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfCompositeLit(ast::CompositeLit* expr,
                                                               Context& ctx) {
  // TODO: implement
  return nullptr;
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfBasicLit(ast::BasicLit* basic_lit) {
  types::ExprInfo lit_info = type_info_->ExprInfoOf(basic_lit).value();
  types::Basic* basic_type = static_cast<types::Basic*>(lit_info.type());
  return ToIRConstant(basic_type, lit_info.constant_value());
}

std::shared_ptr<ir::Value> IRBuilder::BuildAddressOfIdent(ast::Ident* ident, Context& ctx) {
  types::Object* object = type_info_->ObjectOf(ident);
  types::Variable* var = static_cast<types::Variable*>(object);
  std::shared_ptr<ir::Value> address = ctx.LookupAddressOfVar(var);
  return address;
}

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfIdent(ast::Ident* ident, Context& ctx) {
  types::Object* object = type_info_->ObjectOf(ident);
  types::ExprInfo ident_info = type_info_->ExprInfoOf(ident).value();
  switch (object->object_kind()) {
    case types::ObjectKind::kConstant: {
      types::Constant* constant = static_cast<types::Constant*>(object);
      types::Basic* type = static_cast<types::Basic*>(constant->type());
      return ToIRConstant(type, ident_info.constant_value());
    }
    case types::ObjectKind::kVariable: {
      types::Variable* var = static_cast<types::Variable*>(object);
      ir::Type* type = ToIRType(var->type());
      std::shared_ptr<ir::Value> address = ctx.LookupAddressOfVar(var);
      std::shared_ptr<ir::Computed> value =
          std::make_shared<ir::Computed>(type, ctx.func()->next_computed_number());
      ctx.block()->instrs().push_back(std::make_unique<ir::LoadInstr>(value, address));
      return value;
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

std::shared_ptr<ir::Value> IRBuilder::BuildValueOfConversion(std::shared_ptr<ir::Value> value,
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

std::shared_ptr<ir::Value> IRBuilder::DefaultIRValueForType(types::Type* lang_type) const {
  switch (lang_type->type_kind()) {
    case types::TypeKind::kBasic: {
      ir::Type* ir_type = ToIRType(static_cast<types::Basic*>(lang_type));
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
      return std::make_shared<ir_ext::StringConstant>(ir_string_type_, "");
      // TODO: implement more types
      // throw "internal error: unexpected lang type";
  }
}

std::shared_ptr<ir::Value> IRBuilder::ToIRConstant(types::Basic* basic,
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

ir::Type* IRBuilder::ToIRType(types::Type* type) const {
  switch (type->type_kind()) {
    case types::TypeKind::kBasic:
      switch (static_cast<types::Basic*>(type)->kind()) {
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
          throw "internal error: unexpected basic type";
      }
    case types::TypeKind::kPointer:
      switch (static_cast<types::Pointer*>(type)->kind()) {
        case types::Pointer::Kind::kWeak:
          return program_->type_table().AtomicOfKind(ir::AtomicKind::kPtr);
        case types::Pointer::Kind::kStrong:
          return ir_ref_count_ptr_type_;
        default:
          throw "internal error: unexpected pointer kind";
      }
    case types::TypeKind::kArray:
    case types::TypeKind::kSlice: {
      types::Container* types_container = static_cast<types::Container*>(type);
      types::Type* types_element = types_container->element_type();
      ir::Type* ir_element = ToIRType(types_element);
      int64_t size = ir_ext::kDynamicArraySize;
      if (type->type_kind() == types::TypeKind::kArray) {
        size = static_cast<types::Array*>(type)->length();
      }
      return program_->type_table().AddType(std::make_unique<ir_ext::Array>(ir_element, size));
    }
    case types::TypeKind::kNamedType:
      return ToIRType(static_cast<types::NamedType*>(type)->underlying());
    case types::TypeKind::kTypeInstance: {
      types::InfoBuilder type_info_builder = type_info_->builder();
      types::Type* underlying =
          types::UnderlyingOf(static_cast<types::TypeInstance*>(type), type_info_builder);
      return ToIRType(underlying);
    }
    case types::TypeKind::kSignature:
      return program_->type_table().AtomicOfKind(ir::AtomicKind::kFunc);
    case types::TypeKind::kStruct: {
      types::Struct* types_struct = static_cast<types::Struct*>(type);
      if (types_struct->is_empty()) {
        return ir_empty_struct_;
      }
      // TODO: handle embdedded structs
      std::vector<std::pair<std::string, ir::Type*>> fields;
      for (types::Variable* types_field : types_struct->fields()) {
        std::string name = types_field->name();
        ir::Type* ir_field_type = ToIRType(types_field->type());
        fields.push_back({name, ir_field_type});
      }
      return program_->type_table().AddType(std::make_unique<ir_ext::Struct>(fields));
    }
    case types::TypeKind::kInterface: {
      types::Interface* types_interface = static_cast<types::Interface*>(type);
      if (types_interface->is_empty()) {
        return ir_empty_interface_;
      }
      // TODO: handle embdedded interfaces
      std::vector<std::string> methods;
      for (types::Func* types_method : types_interface->methods()) {
        methods.push_back(types_method->name());
      }
      return program_->type_table().AddType(std::make_unique<ir_ext::Interface>(methods));
    }
    case types::TypeKind::kTypeParameter:
      return ToIRType(static_cast<types::TypeParameter*>(type)->interface());
    default:
      throw "internal error: unexpected type";
  }
}

}  // namespace ir_builder
}  // namespace lang

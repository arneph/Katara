//
//  expr_builder.cpp
//  Katara
//
//  Created by Arne Philipeit on 7/18/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "expr_builder.h"

#include "src/common/logging/logging.h"

namespace lang {
namespace ir_builder {

std::vector<std::shared_ptr<ir::Computed>> ExprBuilder::BuildAddressesOfExprs(
    std::vector<ast::Expr*> exprs, ASTContext& ast_ctx, IRContext& ir_ctx) {
  std::vector<std::shared_ptr<ir::Computed>> addresses;
  for (auto expr : exprs) {
    addresses.push_back(BuildAddressOfExpr(expr, ast_ctx, ir_ctx));
  }
  return addresses;
}

std::shared_ptr<ir::Computed> ExprBuilder::BuildAddressOfExpr(ast::Expr* expr, ASTContext& ast_ctx,
                                                              IRContext& ir_ctx) {
  switch (expr->node_kind()) {
    case ast::NodeKind::kUnaryExpr:
      return BuildAddressOfUnaryMemoryExpr(static_cast<ast::UnaryExpr*>(expr), ast_ctx, ir_ctx);
    case ast::NodeKind::kSelectionExpr:
      // TODO: handle other selection kinds
      return BuildAddressOfStructFieldSelectionExpr(static_cast<ast::SelectionExpr*>(expr), ast_ctx,
                                                    ir_ctx);
    case ast::NodeKind::kIndexExpr:
      return BuildAddressOfIndexExpr(static_cast<ast::IndexExpr*>(expr), ast_ctx, ir_ctx);
    case ast::NodeKind::kIdent:
      return BuildAddressOfIdent(static_cast<ast::Ident*>(expr), ast_ctx);
    default:
      common::fail("unexpected addressable expr");
  }
}

std::vector<std::shared_ptr<ir::Value>> ExprBuilder::BuildValuesOfExprs(
    std::vector<ast::Expr*> exprs, ASTContext& ast_ctx, IRContext& ir_ctx) {
  std::vector<std::shared_ptr<ir::Value>> values;
  for (auto expr : exprs) {
    std::vector<std::shared_ptr<ir::Value>> expr_values = BuildValuesOfExpr(expr, ast_ctx, ir_ctx);
    if (!expr_values.empty()) {
      values.push_back(expr_values.front());
    }
  }
  return values;
}

std::vector<std::shared_ptr<ir::Value>> ExprBuilder::BuildValuesOfExpr(ast::Expr* expr,
                                                                       ASTContext& ast_ctx,
                                                                       IRContext& ir_ctx) {
  switch (expr->node_kind()) {
    case ast::NodeKind::kUnaryExpr:
      return {BuildValueOfUnaryExpr(static_cast<ast::UnaryExpr*>(expr), ast_ctx, ir_ctx)};
    case ast::NodeKind::kBinaryExpr:
      return {BuildValueOfBinaryExpr(static_cast<ast::BinaryExpr*>(expr), ast_ctx, ir_ctx)};
    case ast::NodeKind::kCompareExpr:
      return {BuildValueOfCompareExpr(static_cast<ast::CompareExpr*>(expr), ast_ctx, ir_ctx)};
    case ast::NodeKind::kParenExpr:
      return BuildValuesOfExpr(static_cast<ast::ParenExpr*>(expr)->x(), ast_ctx, ir_ctx);
    case ast::NodeKind::kSelectionExpr:
      return BuildValuesOfSelectionExpr(static_cast<ast::SelectionExpr*>(expr), ast_ctx, ir_ctx);
    case ast::NodeKind::kTypeAssertExpr:
      return BuildValuesOfTypeAssertExpr(static_cast<ast::TypeAssertExpr*>(expr), ast_ctx, ir_ctx);
    case ast::NodeKind::kIndexExpr:
      return {BuildValueOfIndexExpr(static_cast<ast::IndexExpr*>(expr), ast_ctx, ir_ctx)};
    case ast::NodeKind::kCallExpr:
      return BuildValuesOfCallExpr(static_cast<ast::CallExpr*>(expr), ast_ctx, ir_ctx);
    case ast::NodeKind::kFuncLit:
      return {BuildValueOfFuncLit(static_cast<ast::FuncLit*>(expr), ast_ctx, ir_ctx)};
    case ast::NodeKind::kCompositeLit:
      return {BuildValueOfCompositeLit(static_cast<ast::CompositeLit*>(expr), ast_ctx, ir_ctx)};
    case ast::NodeKind::kBasicLit:
      return {BuildValueOfBasicLit(static_cast<ast::BasicLit*>(expr))};
    case ast::NodeKind::kIdent:
      return {BuildValueOfIdent(static_cast<ast::Ident*>(expr), ast_ctx, ir_ctx)};
    default:
      common::fail("unexpected expr");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfUnaryExpr(ast::UnaryExpr* expr,
                                                              ASTContext& ast_ctx,
                                                              IRContext& ir_ctx) {
  switch (expr->op()) {
    case tokens::kMul:
    case tokens::kRem:
    case tokens::kAnd:
      return BuildValueOfUnaryMemoryExpr(expr, ast_ctx, ir_ctx);
    case tokens::kAdd:
      return BuildValuesOfExpr(expr->x(), ast_ctx, ir_ctx).front();
    case tokens::kSub:
      return BuildValueOfIntUnaryExpr(expr, common::Int::UnaryOp::kNeg, ast_ctx, ir_ctx);
    case tokens::kXor:
      return BuildValueOfIntUnaryExpr(expr, common::Int::UnaryOp::kNot, ast_ctx, ir_ctx);
    case tokens::kNot:
      return BuildValueOfBoolNotExpr(expr, ast_ctx, ir_ctx);
    default:
      common::fail("unexpected unary op");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfBoolNotExpr(ast::UnaryExpr* expr,
                                                                ASTContext& ast_ctx,
                                                                IRContext& ir_ctx) {
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ast_ctx, ir_ctx).front();
  return value_builder_.BuildBoolNot(x, ir_ctx);
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfIntUnaryExpr(ast::UnaryExpr* expr,
                                                                 common::Int::UnaryOp op,
                                                                 ASTContext& ast_ctx,
                                                                 IRContext& ir_ctx) {
  types::Basic* basic_type =
      static_cast<types::Basic*>(type_info_->ExprInfoOf(expr).value().type());
  const ir::Type* ir_type = type_builder_.BuildType(basic_type);
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ast_ctx, ir_ctx).front();
  x = value_builder_.BuildConversion(x, ir_type, ir_ctx);
  return value_builder_.BuildIntUnaryOp(op, x, ir_ctx);
}

std::shared_ptr<ir::Computed> ExprBuilder::BuildAddressOfUnaryMemoryExpr(ast::UnaryExpr* expr,
                                                                         ASTContext& ast_ctx,
                                                                         IRContext& ir_ctx) {
  if (expr->op() == tokens::kMul || expr->op() == tokens::kRem) {
    return std::static_pointer_cast<ir::Computed>(
        BuildValuesOfExpr(expr->x(), ast_ctx, ir_ctx).front());
  } else {
    common::fail("unexpected unary memory expr");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfUnaryMemoryExpr(ast::UnaryExpr* expr,
                                                                    ASTContext& ast_ctx,
                                                                    IRContext& ir_ctx) {
  ast::Expr* x = expr->x();
  if (expr->op() == tokens::kAnd) {
    if (x->node_kind() == ast::NodeKind::kCompositeLit) {
      const ir_ext::SharedPointer* ir_struct_pointer_type =
          type_builder_.BuildStrongPointerToType(type_info_->ExprInfoOf(x)->type());
      std::shared_ptr<ir::Value> struct_value =
          BuildValueOfCompositeLit(static_cast<ast::CompositeLit*>(x), ast_ctx, ir_ctx);
      std::shared_ptr<ir::Computed> struct_address = std::make_shared<ir::Computed>(
          ir_struct_pointer_type, ir_ctx.func()->next_computed_number());
      ir_ctx.block()->instrs().push_back(
          std::make_unique<ir_ext::MakeSharedPointerInstr>(struct_address));
      ir_ctx.block()->instrs().push_back(
          std::make_unique<ir::StoreInstr>(struct_address, struct_value));
      return struct_address;
    } else {
      return BuildAddressOfExpr(x, ast_ctx, ir_ctx);
    }

  } else if (expr->op() == tokens::kMul || expr->op() == tokens::kRem) {
    std::shared_ptr<ir::Value> address = BuildAddressOfExpr(x, ast_ctx, ir_ctx);
    types::Type* types_value_type = type_info_->ExprInfoOf(x)->type();
    const ir::Type* ir_value_type = type_builder_.BuildType(types_value_type);
    std::shared_ptr<ir::Computed> value =
        std::make_shared<ir::Computed>(ir_value_type, ir_ctx.func()->next_computed_number());
    ir_ctx.block()->instrs().push_back(std::make_unique<ir::LoadInstr>(value, address));
    return value;
  } else {
    common::fail("unexpected unary memory expr");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfBinaryExpr(ast::BinaryExpr* expr,
                                                               ASTContext& ast_ctx,
                                                               IRContext& ir_ctx) {
  types::Basic* basic_type = static_cast<types::Basic*>(type_info_->ExprInfoOf(expr)->type());
  switch (expr->op()) {
    case tokens::kAdd:
      if (basic_type->kind() == types::Basic::kUntypedString ||
          basic_type->kind() == types::Basic::kString) {
        return BuildValueOfStringConcatExpr(expr, ast_ctx, ir_ctx);
      }
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kAdd, ast_ctx, ir_ctx);
    case tokens::kSub:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kSub, ast_ctx, ir_ctx);
    case tokens::kMul:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kMul, ast_ctx, ir_ctx);
    case tokens::kQuo:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kDiv, ast_ctx, ir_ctx);
    case tokens::kRem:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kRem, ast_ctx, ir_ctx);
    case tokens::kAnd:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kAnd, ast_ctx, ir_ctx);
    case tokens::kOr:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kOr, ast_ctx, ir_ctx);
    case tokens::kXor:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kXor, ast_ctx, ir_ctx);
    case tokens::kAndNot:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kAndNot, ast_ctx, ir_ctx);
    case tokens::kShl:
      return BuildValueOfIntShiftExpr(expr, common::Int::ShiftOp::kLeft, ast_ctx, ir_ctx);
    case tokens::kShr:
      return BuildValueOfIntShiftExpr(expr, common::Int::ShiftOp::kRight, ast_ctx, ir_ctx);
    case tokens::kLAnd:
    case tokens::kLOr:
      return BuildValueOfBinaryLogicExpr(expr, ast_ctx, ir_ctx);
    default:
      common::fail("unexpected binary op");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfStringConcatExpr(ast::BinaryExpr* expr,
                                                                     ASTContext& ast_ctx,
                                                                     IRContext& ir_ctx) {
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ast_ctx, ir_ctx).front();
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(expr->x(), ast_ctx, ir_ctx).front();
  return value_builder_.BuildStringConcat(x, y, ir_ctx);
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfIntBinaryExpr(ast::BinaryExpr* expr,
                                                                  common::Int::BinaryOp op,
                                                                  ASTContext& ast_ctx,
                                                                  IRContext& ir_ctx) {
  types::Basic* basic_type =
      static_cast<types::Basic*>(type_info_->ExprInfoOf(expr).value().type());
  const ir::Type* ir_type = type_builder_.BuildTypeForBasic(basic_type);
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ast_ctx, ir_ctx).front();
  x = value_builder_.BuildConversion(x, ir_type, ir_ctx);
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(expr->y(), ast_ctx, ir_ctx).front();
  y = value_builder_.BuildConversion(y, ir_type, ir_ctx);
  return value_builder_.BuildIntBinaryOp(x, op, y, ir_ctx);
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfIntShiftExpr(ast::BinaryExpr* expr,
                                                                 common::Int::ShiftOp op,
                                                                 ASTContext& ast_ctx,
                                                                 IRContext& ir_ctx) {
  types::Basic* basic_type =
      static_cast<types::Basic*>(type_info_->ExprInfoOf(expr).value().type());
  const ir::Type* ir_type = type_builder_.BuildTypeForBasic(basic_type);
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ast_ctx, ir_ctx).front();
  x = value_builder_.BuildConversion(x, ir_type, ir_ctx);
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(expr->y(), ast_ctx, ir_ctx).front();
  y = value_builder_.BuildConversion(y, ir::u64(), ir_ctx);
  return value_builder_.BuildIntShiftOp(x, op, y, ir_ctx);
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfBinaryLogicExpr(ast::BinaryExpr* expr,
                                                                    ASTContext& ast_ctx,
                                                                    IRContext& ir_ctx) {
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ast_ctx, ir_ctx).front();
  ir::Block* x_exit_block = ir_ctx.block();

  ir::Block* y_entry_block = ir_ctx.func()->AddBlock();
  IRContext y_ir_ctx = ir_ctx.ChildContextFor(y_entry_block);
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(expr->y(), ast_ctx, y_ir_ctx).front();
  ir::Block* y_exit_block = y_ir_ctx.block();

  ir::Block* merge_block = ir_ctx.func()->AddBlock();
  ir_ctx.set_block(merge_block);

  ir::block_num_t destination_true, destination_false;
  std::shared_ptr<ir::Constant> short_circuit_value;
  switch (expr->op()) {
    case tokens::kLAnd:
      destination_true = y_entry_block->number();
      destination_false = merge_block->number();
      short_circuit_value = ir::False();
      break;
    case tokens::kLOr:
      destination_true = merge_block->number();
      destination_false = y_entry_block->number();
      short_circuit_value = ir::True();
      break;
    default:
      common::fail("unexpected logic op");
  }

  x_exit_block->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(x, destination_true, destination_false));
  y_exit_block->instrs().push_back(std::make_unique<ir::JumpInstr>(merge_block->number()));

  auto result =
      std::make_shared<ir::Computed>(ir::bool_type(), ir_ctx.func()->next_computed_number());
  auto inherited_short_circuit_value =
      std::make_shared<ir::InheritedValue>(short_circuit_value, x_exit_block->number());
  auto inherited_y = std::make_shared<ir::InheritedValue>(y, y_exit_block->number());
  merge_block->instrs().push_back(
      std::make_unique<ir::PhiInstr>(result, std::vector<std::shared_ptr<ir::InheritedValue>>{
                                                 inherited_short_circuit_value, inherited_y}));

  ir_ctx.func()->AddControlFlow(x_exit_block->number(), y_entry_block->number());
  ir_ctx.func()->AddControlFlow(x_exit_block->number(), merge_block->number());
  ir_ctx.func()->AddControlFlow(y_exit_block->number(), merge_block->number());

  return result;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfCompareExpr(ast::CompareExpr* expr,
                                                                ASTContext& ast_ctx,
                                                                IRContext& ir_ctx) {
  if (expr->compare_ops().size() == 1) {
    return {BuildValueOfSingleCompareExpr(expr, ast_ctx, ir_ctx)};
  } else {
    return {BuildValueOfMultipleCompareExpr(expr, ast_ctx, ir_ctx)};
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfSingleCompareExpr(ast::CompareExpr* expr,
                                                                      ASTContext& ast_ctx,
                                                                      IRContext& ir_ctx) {
  ast::Expr* x_expr = expr->operands().front();
  types::Type* x_type = type_info_->ExprInfoOf(x_expr)->type();
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(x_expr, ast_ctx, ir_ctx).front();

  ast::Expr* y_expr = expr->operands().back();
  types::Type* y_type = type_info_->ExprInfoOf(y_expr)->type();
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(y_expr, ast_ctx, ir_ctx).front();

  return BuildValueOfComparison(expr->compare_ops().front(), x, x_type, y, y_type, ast_ctx, ir_ctx);
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfMultipleCompareExpr(ast::CompareExpr* expr,
                                                                        ASTContext& ast_ctx,
                                                                        IRContext& ir_ctx) {
  ast::Expr* x_expr = expr->operands().front();
  types::Type* x_type = type_info_->ExprInfoOf(x_expr)->type();
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(x_expr, ast_ctx, ir_ctx).front();

  tokens::Token op = expr->compare_ops().front();
  ast::Expr* y_expr = expr->operands().at(1);
  types::Type* y_type = type_info_->ExprInfoOf(y_expr)->type();
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(y_expr, ast_ctx, ir_ctx).front();

  std::shared_ptr<ir::Value> partial_result =
      BuildValueOfComparison(op, x, x_type, y, y_type, ast_ctx, ir_ctx);

  ir::Block* prior_block = ir_ctx.block();
  ir::Block* merge_block = ir_ctx.func()->AddBlock();

  std::vector<std::shared_ptr<ir::InheritedValue>> merge_values;

  for (size_t i = 1; i < expr->compare_ops().size(); i++) {
    ir::Block* start_block = ir_ctx.func()->AddBlock();
    ir_ctx.set_block(start_block);

    prior_block->instrs().push_back(std::make_unique<ir::JumpCondInstr>(
        partial_result, start_block->number(), merge_block->number()));
    ir_ctx.func()->AddControlFlow(prior_block->number(), start_block->number());
    ir_ctx.func()->AddControlFlow(prior_block->number(), merge_block->number());
    merge_values.push_back(
        std::make_shared<ir::InheritedValue>(ir::False(), prior_block->number()));

    x_expr = y_expr;
    x_type = y_type;
    x = y;

    op = expr->compare_ops().at(i);
    y_expr = expr->operands().at(i + 1);
    y_type = type_info_->ExprInfoOf(y_expr)->type();
    y = BuildValuesOfExpr(y_expr, ast_ctx, ir_ctx).front();

    partial_result = BuildValueOfComparison(op, x, x_type, y, y_type, ast_ctx, ir_ctx);
    prior_block = ir_ctx.block();

    if (i == expr->compare_ops().size() - 1) {
      prior_block->instrs().push_back(std::make_unique<ir::JumpInstr>(merge_block->number()));
      ir_ctx.func()->AddControlFlow(prior_block->number(), merge_block->number());
      merge_values.push_back(
          std::make_shared<ir::InheritedValue>(partial_result, prior_block->number()));
    }
  }

  ir_ctx.set_block(merge_block);

  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir::bool_type(), ir_ctx.func()->next_computed_number());
  merge_block->instrs().push_back(std::make_unique<ir::PhiInstr>(result, merge_values));

  return result;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfComparison(
    tokens::Token op, std::shared_ptr<ir::Value> x, types::Type* x_type,
    std::shared_ptr<ir::Value> y, types::Type* y_type, ASTContext& ast_ctx, IRContext& ir_ctx) {
  types::InfoBuilder info_builder = type_info_->builder();
  types::Type* x_underlying_type = types::UnderlyingOf(x_type, info_builder);
  types::Type* y_underlying_type = types::UnderlyingOf(y_type, info_builder);
  if (x_underlying_type->type_kind() == types::TypeKind::kBasic &&
      y_underlying_type->type_kind() == types::TypeKind::kBasic) {
    types::Basic* x_basic_type = static_cast<types::Basic*>(x_underlying_type);
    if (x_basic_type->info() & types::Basic::kIsBoolean) {
      return BuildValueOfBoolComparison(op, x, y, ir_ctx);
    } else if (x_basic_type->info() & types::Basic::kIsInteger) {
      return BuildValueOfIntComparison(op, x, y, ir_ctx);
    } else if (x_basic_type->info() & types::Basic::kIsString) {
      return value_builder_.BuildStringComparison(x, op, y, ir_ctx);
    }
  }

  // TODO: implement
  return ir::True();
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfBoolComparison(tokens::Token tok,
                                                                   std::shared_ptr<ir::Value> x,
                                                                   std::shared_ptr<ir::Value> y,
                                                                   IRContext& ir_ctx) {
  common::Bool::BinaryOp op = [tok]() {
    switch (tok) {
      case tokens::kEql:
        return common::Bool::BinaryOp::kEq;
      case tokens::kNeq:
        return common::Bool::BinaryOp::kNeq;
      default:
        common::fail("unexpected bool comparison op");
    }
  }();
  return value_builder_.BuildBoolBinaryOp(x, op, y, ir_ctx);
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfIntComparison(tokens::Token tok,
                                                                  std::shared_ptr<ir::Value> x,
                                                                  std::shared_ptr<ir::Value> y,
                                                                  IRContext& ir_ctx) {
  common::Int::CompareOp op = [tok]() {
    switch (tok) {
      case tokens::kEql:
        return common::Int::CompareOp::kEq;
      case tokens::kNeq:
        return common::Int::CompareOp::kNeq;
      case tokens::kLss:
        return common::Int::CompareOp::kLss;
      case tokens::kLeq:
        return common::Int::CompareOp::kLeq;
      case tokens::kGeq:
        return common::Int::CompareOp::kGeq;
      case tokens::kGtr:
        return common::Int::CompareOp::kGtr;
      default:
        common::fail("unexpected int comparison op");
    }
  }();
  common::IntType x_type = static_cast<const ir::IntType*>(x->type())->int_type();
  common::IntType y_type = static_cast<const ir::IntType*>(y->type())->int_type();
  if (common::BitSizeOf(x_type) > common::BitSizeOf(y_type) || common::IsUnsigned(x_type)) {
    y = value_builder_.BuildConversion(y, ir::IntTypeFor(x_type), ir_ctx);
  } else {
    x = value_builder_.BuildConversion(x, ir::IntTypeFor(y_type), ir_ctx);
  }
  return value_builder_.BuildIntCompareOp(x, op, y, ir_ctx);
}

std::vector<std::shared_ptr<ir::Value>> ExprBuilder::BuildValuesOfSelectionExpr(
    ast::SelectionExpr* expr, ASTContext& ast_ctx, IRContext& ir_ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Computed> ExprBuilder::BuildAddressOfStructFieldSelectionExpr(
    ast::SelectionExpr* expr, ASTContext& ast_ctx, IRContext& ir_ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfStructFieldSelectionExpr(
    ast::SelectionExpr* expr, ASTContext& ast_ctx, IRContext& ir_ctx) {
  // TODO: implement
  return {};
}

std::vector<std::shared_ptr<ir::Value>> ExprBuilder::BuildValuesOfTypeAssertExpr(
    ast::TypeAssertExpr* expr, ASTContext& ast_ctx, IRContext& ir_ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Computed> ExprBuilder::BuildAddressOfIndexExpr(ast::IndexExpr* expr,
                                                                   ASTContext& ast_ctx,
                                                                   IRContext& ir_ctx) {
  types::Type* types_element_type = type_info_->ExprInfoOf(expr)->type();
  const ir_ext::SharedPointer* ir_pointer_type =
      type_builder_.BuildWeakPointerToType(types_element_type);
  // TODO: implement (array, slice)
  std::shared_ptr<ir::Computed> address =
      std::make_shared<ir::Computed>(ir_pointer_type, ir_ctx.func()->next_computed_number());
  return address;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfIndexExpr(ast::IndexExpr* expr,
                                                              ASTContext& ast_ctx,
                                                              IRContext& ir_ctx) {
  ast::Expr* accessed_expr = expr->accessed();
  ast::Expr* index_expr = expr->index();
  types::InfoBuilder info_builder = type_info_->builder();
  types::Type* types_accessed_type = type_info_->ExprInfoOf(accessed_expr)->type();
  types::Type* types_accessed_underlying_type =
      types::UnderlyingOf(types_accessed_type, info_builder);
  if (types_accessed_underlying_type->type_kind() == types::TypeKind::kBasic) {
    // Note: strings are the only basic type that can be indexed
    const ir::Type* rune_type = ir::i32();
    std::shared_ptr<ir::Value> string = BuildValuesOfExpr(accessed_expr, ast_ctx, ir_ctx).front();
    std::shared_ptr<ir::Value> index = BuildValuesOfExpr(index_expr, ast_ctx, ir_ctx).front();
    std::shared_ptr<ir::Computed> value =
        std::make_shared<ir::Computed>(rune_type, ir_ctx.func()->next_computed_number());
    ir_ctx.block()->instrs().push_back(
        std::make_unique<ir_ext::StringIndexInstr>(value, string, index));
    return value;

  } else if (types_accessed_underlying_type->is_container()) {
    types::Type* types_element_type = type_info_->ExprInfoOf(expr)->type();
    const ir::Type* ir_element_type = type_builder_.BuildType(types_element_type);
    std::shared_ptr<ir::Computed> value =
        std::make_shared<ir::Computed>(ir_element_type, ir_ctx.func()->next_computed_number());
    // TODO: actually add load instr
    return value;
  } else {
    common::fail("unexpected accessed value in index expr");
  }
}

std::vector<std::shared_ptr<ir::Value>> ExprBuilder::BuildValuesOfCallExpr(ast::CallExpr* expr,
                                                                           ASTContext& ast_ctx,
                                                                           IRContext& ir_ctx) {
  switch (type_info_->ExprInfoOf(expr->func())->kind()) {
    case types::ExprInfo::Kind::kBuiltin:
      return {BuildValueOfCallExprWithTypeConversion(expr, ast_ctx, ir_ctx)};
    case types::ExprInfo::Kind::kType:
      return BuildValuesOfCallExprWithBuiltin(expr, ast_ctx, ir_ctx);
    case types::ExprInfo::Kind::kVariable:
    case types::ExprInfo::Kind::kValue:
    case types::ExprInfo::Kind::kValueOk:
      return BuildValuesOfCallExprWithFuncCall(expr, ast_ctx, ir_ctx);
    default:
      common::fail("unexpected func expr kind in call expr");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfCallExprWithTypeConversion(ast::CallExpr* expr,
                                                                               ASTContext& ast_ctx,
                                                                               IRContext& ir_ctx) {
  // TODO: implement
  return {};
}

std::vector<std::shared_ptr<ir::Value>> ExprBuilder::BuildValuesOfCallExprWithBuiltin(
    ast::CallExpr* expr, ASTContext& ast_ctx, IRContext& ir_ctx) {
  // TODO: implement
  return {};
}

std::vector<std::shared_ptr<ir::Value>> ExprBuilder::BuildValuesOfCallExprWithFuncCall(
    ast::CallExpr* expr, ASTContext& ast_ctx, IRContext& ir_ctx) {
  std::shared_ptr<ir::Value> ir_func = BuildValuesOfExpr(expr->func(), ast_ctx, ir_ctx).front();
  // TODO: support type parameters
  // TODO: support receivers

  types::Type* types_expr_type = type_info_->TypeOf(expr);
  std::vector<std::shared_ptr<ir::Value>> args = BuildValuesOfExprs(expr->args(), ast_ctx, ir_ctx);
  std::vector<std::shared_ptr<ir::Computed>> results;
  if (types_expr_type->type_kind() == types::TypeKind::kTuple) {
    auto types_tuple = static_cast<types::Tuple*>(types_expr_type);
    results.reserve(types_tuple->variables().size());
    for (types::Variable* types_tuple_member : types_tuple->variables()) {
      types::Type* types_result_type = types_tuple_member->type();
      const ir::Type* ir_result_type = type_builder_.BuildType(types_result_type);
      results.push_back(
          std::make_shared<ir::Computed>(ir_result_type, ir_ctx.func()->next_computed_number()));
    }
  } else {
    const ir::Type* ir_result_type = type_builder_.BuildType(types_expr_type);
    results.push_back(
        std::make_shared<ir::Computed>(ir_result_type, ir_ctx.func()->next_computed_number()));
  }
  ir_ctx.block()->instrs().push_back(std::make_unique<ir::CallInstr>(ir_func, results, args));
  std::vector<std::shared_ptr<ir::Value>> result_values;
  result_values.reserve(results.size());
  for (std::shared_ptr<ir::Computed>& result : results) {
    result_values.push_back(std::static_pointer_cast<ir::Value>(result));
  }
  return static_cast<std::vector<std::shared_ptr<ir::Value>>>(result_values);
}

std::shared_ptr<ir::Constant> ExprBuilder::BuildValueOfFuncLit(ast::FuncLit* expr,
                                                               ASTContext& ast_ctx,
                                                               IRContext& ir_ctx) {
  // TODO: implement
  return nullptr;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfCompositeLit(ast::CompositeLit* expr,
                                                                 ASTContext& ast_ctx,
                                                                 IRContext& ir_ctx) {
  // TODO: implement
  return nullptr;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfBasicLit(ast::BasicLit* basic_lit) {
  return value_builder_.BuildConstant(type_info_->ExprInfoOf(basic_lit).value().constant_value());
}

std::shared_ptr<ir::Computed> ExprBuilder::BuildAddressOfIdent(ast::Ident* ident,
                                                               ASTContext& ast_ctx) {
  types::Object* object = type_info_->ObjectOf(ident);
  types::Variable* var = static_cast<types::Variable*>(object);
  return ast_ctx.LookupAddressOfVar(var);
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfIdent(ast::Ident* ident, ASTContext& ast_ctx,
                                                          IRContext& ir_ctx) {
  types::Object* object = type_info_->ObjectOf(ident);
  types::ExprInfo ident_info = type_info_->ExprInfoOf(ident).value();
  switch (object->object_kind()) {
    case types::ObjectKind::kConstant: {
      types::Constant* constant = static_cast<types::Constant*>(object);
      return value_builder_.BuildConstant(constant->value());
    }
    case types::ObjectKind::kVariable: {
      types::Variable* var = static_cast<types::Variable*>(object);
      const ir::Type* type = type_builder_.BuildType(var->type());
      std::shared_ptr<ir::Value> address = ast_ctx.LookupAddressOfVar(var);
      std::shared_ptr<ir::Computed> value =
          std::make_shared<ir::Computed>(type, ir_ctx.func()->next_computed_number());
      ir_ctx.block()->instrs().push_back(std::make_unique<ir::LoadInstr>(value, address));
      return value;
    }
    case types::ObjectKind::kFunc: {
      types::Func* types_func = static_cast<types::Func*>(object);
      ir::Func* ir_func = funcs_.at(types_func);
      return ir::ToFuncConstant(ir_func->number());
    }
    case types::ObjectKind::kNil: {
      return ir::NilPointer();
    }
    default:
      common::fail("unexpected object kind");
  }
}

}  // namespace ir_builder
}  // namespace lang

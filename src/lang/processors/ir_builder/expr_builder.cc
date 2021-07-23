//
//  expr_builder.cpp
//  Katara
//
//  Created by Arne Philipeit on 7/18/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "expr_builder.h"

#include "src/common/logging.h"

namespace lang {
namespace ir_builder {

std::vector<std::shared_ptr<ir::Computed>> ExprBuilder::BuildAddressesOfExprs(
    std::vector<ast::Expr*> exprs, Context& ctx) {
  std::vector<std::shared_ptr<ir::Computed>> addresses;
  for (auto expr : exprs) {
    addresses.push_back(BuildAddressOfExpr(expr, ctx));
  }
  return addresses;
}

std::shared_ptr<ir::Computed> ExprBuilder::BuildAddressOfExpr(ast::Expr* expr, Context& ctx) {
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
      common::fail("unexpected addressable expr");
  }
}

std::vector<std::shared_ptr<ir::Value>> ExprBuilder::BuildValuesOfExprs(
    std::vector<ast::Expr*> exprs, Context& ctx) {
  std::vector<std::shared_ptr<ir::Value>> values;
  for (auto expr : exprs) {
    std::vector<std::shared_ptr<ir::Value>> expr_values = BuildValuesOfExpr(expr, ctx);
    if (!expr_values.empty()) {
      values.push_back(expr_values.front());
    }
  }
  return values;
}

std::vector<std::shared_ptr<ir::Value>> ExprBuilder::BuildValuesOfExpr(ast::Expr* expr,
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
      common::fail("unexpected expr");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfUnaryExpr(ast::UnaryExpr* expr, Context& ctx) {
  switch (expr->op()) {
    case tokens::kMul:
    case tokens::kRem:
    case tokens::kAnd:
      return BuildValueOfUnaryMemoryExpr(expr, ctx);
    case tokens::kAdd:
      return BuildValuesOfExpr(expr->x(), ctx).front();
    case tokens::kSub:
      return BuildValueOfIntUnaryExpr(expr, common::Int::UnaryOp::kNeg, ctx);
    case tokens::kXor:
      return BuildValueOfIntUnaryExpr(expr, common::Int::UnaryOp::kNot, ctx);
    case tokens::kNot:
      return BuildValueOfBoolNotExpr(expr, ctx);
    default:
      common::fail("unexpected unary op");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfBoolNotExpr(ast::UnaryExpr* expr,
                                                                Context& ctx) {
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ctx).front();
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(&ir::kBool, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir::BoolNotInstr>(result, x));
  return result;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfIntUnaryExpr(ast::UnaryExpr* expr,
                                                                 common::Int::UnaryOp op,
                                                                 Context& ctx) {
  types::Basic* basic_type =
      static_cast<types::Basic*>(type_info_->ExprInfoOf(expr).value().type());
  const ir::Type* ir_type = type_builder_.BuildType(basic_type);
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ctx).front();
  x = BuildValueOfConversion(x, ir_type, ctx);
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_type, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir::IntUnaryInstr>(result, op, x));
  return result;
}

std::shared_ptr<ir::Computed> ExprBuilder::BuildAddressOfUnaryMemoryExpr(ast::UnaryExpr* expr,
                                                                         Context& ctx) {
  if (expr->op() == tokens::kMul || expr->op() == tokens::kRem) {
    return std::static_pointer_cast<ir::Computed>(BuildValuesOfExpr(expr->x(), ctx).front());
  } else {
    common::fail("unexpected unary memory expr");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfUnaryMemoryExpr(ast::UnaryExpr* expr,
                                                                    Context& ctx) {
  ast::Expr* x = expr->x();
  if (expr->op() == tokens::kAnd) {
    if (x->node_kind() == ast::NodeKind::kCompositeLit) {
      const ir_ext::SharedPointer* ir_struct_pointer_type =
          type_builder_.BuildStrongPointerToType(type_info_->ExprInfoOf(x)->type());
      std::shared_ptr<ir::Value> struct_value =
          BuildValueOfCompositeLit(static_cast<ast::CompositeLit*>(x), ctx);
      std::shared_ptr<ir::Computed> struct_address = std::make_shared<ir::Computed>(
          ir_struct_pointer_type, ctx.func()->next_computed_number());
      ctx.block()->instrs().push_back(
          std::make_unique<ir_ext::MakeSharedPointerInstr>(struct_address));
      ctx.block()->instrs().push_back(
          std::make_unique<ir::StoreInstr>(struct_address, struct_value));
      return struct_address;
    } else {
      return BuildAddressOfExpr(x, ctx);
    }

  } else if (expr->op() == tokens::kMul || expr->op() == tokens::kRem) {
    std::shared_ptr<ir::Value> address = BuildAddressOfExpr(x, ctx);
    types::Type* types_value_type = type_info_->ExprInfoOf(x)->type();
    const ir::Type* ir_value_type = type_builder_.BuildType(types_value_type);
    std::shared_ptr<ir::Computed> value =
        std::make_shared<ir::Computed>(ir_value_type, ctx.func()->next_computed_number());
    ctx.block()->instrs().push_back(std::make_unique<ir::LoadInstr>(value, address));
    return value;
  } else {
    common::fail("unexpected unary memory expr");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfBinaryExpr(ast::BinaryExpr* expr,
                                                               Context& ctx) {
  types::Basic* basic_type = static_cast<types::Basic*>(type_info_->ExprInfoOf(expr)->type());
  switch (expr->op()) {
    case tokens::kAdd:
      if (basic_type->kind() == types::Basic::kUntypedString ||
          basic_type->kind() == types::Basic::kString) {
        return BuildValueOfStringConcatExpr(expr, ctx);
      }
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kAdd, ctx);
    case tokens::kSub:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kSub, ctx);
    case tokens::kMul:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kMul, ctx);
    case tokens::kQuo:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kDiv, ctx);
    case tokens::kRem:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kRem, ctx);
    case tokens::kAnd:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kAnd, ctx);
    case tokens::kOr:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kOr, ctx);
    case tokens::kXor:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kXor, ctx);
    case tokens::kAndNot:
      return BuildValueOfIntBinaryExpr(expr, common::Int::BinaryOp::kAndNot, ctx);
    case tokens::kShl:
      return BuildValueOfIntShiftExpr(expr, common::Int::ShiftOp::kLeft, ctx);
    case tokens::kShr:
      return BuildValueOfIntShiftExpr(expr, common::Int::ShiftOp::kRight, ctx);
    case tokens::kLAnd:
    case tokens::kLOr:
      return BuildValueOfBinaryLogicExpr(expr, ctx);
    default:
      common::fail("unexpected binary op");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfStringConcatExpr(ast::BinaryExpr* expr,
                                                                     Context& ctx) {
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ctx).front();
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(expr->x(), ctx).front();
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(&ir_ext::kString, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir_ext::StringConcatInstr>(
      result, std::vector<std::shared_ptr<ir::Value>>{x, y}));
  return result;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfIntBinaryExpr(ast::BinaryExpr* expr,
                                                                  common::Int::BinaryOp op,
                                                                  Context& ctx) {
  types::Basic* basic_type =
      static_cast<types::Basic*>(type_info_->ExprInfoOf(expr).value().type());
  const ir::Type* ir_type = type_builder_.BuildTypeForBasic(basic_type);
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ctx).front();
  x = BuildValueOfConversion(x, ir_type, ctx);
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(expr->y(), ctx).front();
  y = BuildValueOfConversion(y, ir_type, ctx);
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_type, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir::IntBinaryInstr>(result, op, x, y));
  return result;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfIntShiftExpr(ast::BinaryExpr* expr,
                                                                 common::Int::ShiftOp op,
                                                                 Context& ctx) {
  types::Basic* basic_type =
      static_cast<types::Basic*>(type_info_->ExprInfoOf(expr).value().type());
  const ir::Type* ir_type = type_builder_.BuildTypeForBasic(basic_type);
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(expr->x(), ctx).front();
  x = BuildValueOfConversion(x, ir_type, ctx);
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(expr->y(), ctx).front();
  y = BuildValueOfConversion(y, &ir::kU64, ctx);
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(ir_type, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir::IntShiftInstr>(result, op, x, y));
  return result;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfBinaryLogicExpr(ast::BinaryExpr* expr,
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
  std::shared_ptr<ir::Constant> short_circuit_value;
  switch (expr->op()) {
    case tokens::kLAnd:
      destination_true = y_entry_block->number();
      destination_false = merge_block->number();
      short_circuit_value = std::make_shared<ir::BoolConstant>(false);
      break;
    case tokens::kLOr:
      destination_true = merge_block->number();
      destination_false = y_entry_block->number();
      short_circuit_value = std::make_shared<ir::BoolConstant>(true);
      break;
    default:
      common::fail("unexpected logic op");
  }

  x_exit_block->instrs().push_back(
      std::make_unique<ir::JumpCondInstr>(x, destination_true, destination_false));
  y_exit_block->instrs().push_back(std::make_unique<ir::JumpInstr>(merge_block->number()));

  auto result = std::make_shared<ir::Computed>(&ir::kBool, ctx.func()->next_computed_number());
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

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfCompareExpr(ast::CompareExpr* expr,
                                                                Context& ctx) {
  if (expr->compare_ops().size() == 1) {
    return {BuildValueOfSingleCompareExpr(expr, ctx)};
  } else {
    return {BuildValueOfMultipleCompareExpr(expr, ctx)};
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfSingleCompareExpr(ast::CompareExpr* expr,
                                                                      Context& ctx) {
  ast::Expr* x_expr = expr->operands().front();
  types::Type* x_type = type_info_->ExprInfoOf(x_expr)->type();
  std::shared_ptr<ir::Value> x = BuildValuesOfExpr(x_expr, ctx).front();

  ast::Expr* y_expr = expr->operands().back();
  types::Type* y_type = type_info_->ExprInfoOf(y_expr)->type();
  std::shared_ptr<ir::Value> y = BuildValuesOfExpr(y_expr, ctx).front();

  return BuildValueOfComparison(expr->compare_ops().front(), x, x_type, y, y_type, ctx);
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfMultipleCompareExpr(ast::CompareExpr* expr,
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

  std::shared_ptr<ir::BoolConstant> false_value = std::make_shared<ir::BoolConstant>(false);
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
      std::make_shared<ir::Computed>(&ir::kBool, ctx.func()->next_computed_number());
  merge_block->instrs().push_back(std::make_unique<ir::PhiInstr>(result, merge_values));

  return result;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfComparison(tokens::Token op,
                                                               std::shared_ptr<ir::Value> x,
                                                               types::Type* x_type,
                                                               std::shared_ptr<ir::Value> y,
                                                               types::Type* y_type, Context& ctx) {
  types::InfoBuilder info_builder = type_info_->builder();
  types::Type* x_underlying_type = types::UnderlyingOf(x_type, info_builder);
  types::Type* y_underlying_type = types::UnderlyingOf(y_type, info_builder);
  if (x_underlying_type->type_kind() == types::TypeKind::kBasic &&
      y_underlying_type->type_kind() == types::TypeKind::kBasic) {
    types::Basic* x_basic_type = static_cast<types::Basic*>(x_underlying_type);
    if (x_basic_type->info() & types::Basic::kIsBoolean) {
      return BuildValueOfBoolComparison(op, x, y, ctx);
    } else if (x_basic_type->info() & types::Basic::kIsInteger) {
      return BuildValueOfIntComparison(op, x, y, ctx);
    } else if (x_basic_type->info() & types::Basic::kIsString) {
      return BuildValueOfStringComparison(op, x, y, ctx);
    }
  }

  // TODO: implement
  return {std::make_shared<ir::BoolConstant>(true)};
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfBoolComparison(tokens::Token tok,
                                                                   std::shared_ptr<ir::Value> x,
                                                                   std::shared_ptr<ir::Value> y,
                                                                   Context& ctx) {
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
  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(&ir::kBool, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir::BoolBinaryInstr>(result, op, x, y));
  return result;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfIntComparison(tokens::Token tok,
                                                                  std::shared_ptr<ir::Value> x,
                                                                  std::shared_ptr<ir::Value> y,
                                                                  Context& ctx) {
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
    y = BuildValueOfConversion(y, ir::IntTypeFor(x_type), ctx);
  } else {
    x = BuildValueOfConversion(x, ir::IntTypeFor(y_type), ctx);
  }

  std::shared_ptr<ir::Computed> result =
      std::make_shared<ir::Computed>(&ir::kBool, ctx.func()->next_computed_number());
  ctx.block()->instrs().push_back(std::make_unique<ir::IntCompareInstr>(result, op, x, y));
  return result;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfStringComparison(tokens::Token op,
                                                                     std::shared_ptr<ir::Value> x,
                                                                     std::shared_ptr<ir::Value> y,
                                                                     Context& ctx) {
  // TODO: implement
  return {std::make_shared<ir::BoolConstant>(true)};
}

std::vector<std::shared_ptr<ir::Value>> ExprBuilder::BuildValuesOfSelectionExpr(
    ast::SelectionExpr* expr, Context& ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Computed> ExprBuilder::BuildAddressOfStructFieldSelectionExpr(
    ast::SelectionExpr* expr, Context& ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfStructFieldSelectionExpr(
    ast::SelectionExpr* expr, Context& ctx) {
  // TODO: implement
  return {};
}

std::vector<std::shared_ptr<ir::Value>> ExprBuilder::BuildValuesOfTypeAssertExpr(
    ast::TypeAssertExpr* expr, Context& ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Computed> ExprBuilder::BuildAddressOfIndexExpr(ast::IndexExpr* expr,
                                                                   Context& ctx) {
  types::Type* types_element_type = type_info_->ExprInfoOf(expr)->type();
  const ir_ext::SharedPointer* ir_pointer_type =
      type_builder_.BuildWeakPointerToType(types_element_type);
  // TODO: implement (array, slice)
  std::shared_ptr<ir::Computed> address =
      std::make_shared<ir::Computed>(ir_pointer_type, ctx.func()->next_computed_number());
  return address;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfIndexExpr(ast::IndexExpr* expr, Context& ctx) {
  ast::Expr* accessed_expr = expr->accessed();
  ast::Expr* index_expr = expr->index();
  types::InfoBuilder info_builder = type_info_->builder();
  types::Type* types_accessed_type = type_info_->ExprInfoOf(accessed_expr)->type();
  types::Type* types_accessed_underlying_type =
      types::UnderlyingOf(types_accessed_type, info_builder);
  if (types_accessed_underlying_type->type_kind() == types::TypeKind::kBasic) {
    // Note: strings are the only basic type that can be indexed
    const ir::Type* rune_type = &ir::kI32;
    std::shared_ptr<ir::Value> string = BuildValuesOfExpr(accessed_expr, ctx).front();
    std::shared_ptr<ir::Value> index = BuildValuesOfExpr(index_expr, ctx).front();
    std::shared_ptr<ir::Computed> value =
        std::make_shared<ir::Computed>(rune_type, ctx.func()->next_computed_number());
    ctx.block()->instrs().push_back(
        std::make_unique<ir_ext::StringIndexInstr>(value, string, index));
    return value;

  } else if (types_accessed_underlying_type->is_container()) {
    types::Type* types_element_type = type_info_->ExprInfoOf(expr)->type();
    const ir::Type* ir_element_type = type_builder_.BuildType(types_element_type);
    std::shared_ptr<ir::Computed> value =
        std::make_shared<ir::Computed>(ir_element_type, ctx.func()->next_computed_number());
    // TODO: actually add load instr
    return value;
  } else {
    common::fail("unexpected accessed value in index expr");
  }
}

std::vector<std::shared_ptr<ir::Value>> ExprBuilder::BuildValuesOfCallExpr(ast::CallExpr* expr,
                                                                           Context& ctx) {
  // TODO: implement
  return {};
}

std::shared_ptr<ir::Constant> ExprBuilder::BuildValueOfFuncLit(ast::FuncLit* expr, Context& ctx) {
  // TODO: implement
  return nullptr;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfCompositeLit(ast::CompositeLit* expr,
                                                                 Context& ctx) {
  // TODO: implement
  return nullptr;
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfBasicLit(ast::BasicLit* basic_lit) {
  return ToIRConstant(type_info_->ExprInfoOf(basic_lit).value().constant_value());
}

std::shared_ptr<ir::Computed> ExprBuilder::BuildAddressOfIdent(ast::Ident* ident, Context& ctx) {
  types::Object* object = type_info_->ObjectOf(ident);
  types::Variable* var = static_cast<types::Variable*>(object);
  return ctx.LookupAddressOfVar(var);
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfIdent(ast::Ident* ident, Context& ctx) {
  types::Object* object = type_info_->ObjectOf(ident);
  types::ExprInfo ident_info = type_info_->ExprInfoOf(ident).value();
  switch (object->object_kind()) {
    case types::ObjectKind::kConstant: {
      types::Constant* constant = static_cast<types::Constant*>(object);
      return ToIRConstant(constant->value());
    }
    case types::ObjectKind::kVariable: {
      types::Variable* var = static_cast<types::Variable*>(object);
      const ir::Type* type = type_builder_.BuildType(var->type());
      std::shared_ptr<ir::Value> address = ctx.LookupAddressOfVar(var);
      std::shared_ptr<ir::Computed> value =
          std::make_shared<ir::Computed>(type, ctx.func()->next_computed_number());
      ctx.block()->instrs().push_back(std::make_unique<ir::LoadInstr>(value, address));
      return value;
    }
    case types::ObjectKind::kFunc: {
      types::Func* types_func = static_cast<types::Func*>(object);
      ir::Func* ir_func = funcs_.at(types_func);
      return std::make_shared<ir::FuncConstant>(ir_func->number());
    }
    case types::ObjectKind::kNil: {
      return std::make_shared<ir::PointerConstant>(0);
    }
    default:
      common::fail("unexpected object kind");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::BuildValueOfConversion(std::shared_ptr<ir::Value> value,
                                                               const ir::Type* desired_type,
                                                               Context& ctx) {
  if (value->type() == desired_type) {
    return value;
  } else if (ir::IsAtomicType(value->type()->type_kind()) &&
             ir::IsAtomicType(desired_type->type_kind())) {
    std::shared_ptr<ir::Computed> result =
        std::make_shared<ir::Computed>(desired_type, ctx.func()->next_computed_number());
    ctx.block()->instrs().push_back(std::make_unique<ir::Conversion>(result, value));
    return result;
  } else {
    common::fail("unexpected conversion");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::DefaultIRValueForType(types::Type* types_type) {
  switch (types_type->type_kind()) {
    case types::TypeKind::kBasic: {
      const ir::Type* ir_type = type_builder_.BuildType(types_type);
      switch (ir_type->type_kind()) {
        case ir::TypeKind::kBool:
          return std::make_shared<ir::BoolConstant>(false);
        case ir::TypeKind::kInt:
          switch (static_cast<const ir::IntType*>(ir_type)->int_type()) {
            case common::IntType::kI8:
              return std::make_shared<ir::IntConstant>(common::Int(int8_t{0}));
            case common::IntType::kI16:
              return std::make_shared<ir::IntConstant>(common::Int(int16_t{0}));
            case common::IntType::kI32:
              return std::make_shared<ir::IntConstant>(common::Int(int32_t{0}));
            case common::IntType::kI64:
              return std::make_shared<ir::IntConstant>(common::Int(int64_t{0}));
            case common::IntType::kU8:
              return std::make_shared<ir::IntConstant>(common::Int(uint8_t{0}));
            case common::IntType::kU16:
              return std::make_shared<ir::IntConstant>(common::Int(uint16_t{0}));
            case common::IntType::kU32:
              return std::make_shared<ir::IntConstant>(common::Int(uint32_t{0}));
            case common::IntType::kU64:
              return std::make_shared<ir::IntConstant>(common::Int(uint64_t{0}));
          }
        case ir::TypeKind::kPointer:
          return std::make_shared<ir::PointerConstant>(0);
        case ir::TypeKind::kFunc:
          return std::make_shared<ir::FuncConstant>(0);
        case ir::TypeKind::kLangString:
          return std::make_shared<ir_ext::StringConstant>("");
        default:
          common::fail("unexpected ir type for basic type");
      }
    }
    default:
      return std::make_shared<ir_ext::StringConstant>("");
      // TODO: implement more types
      // common::fail("unexpected lang type");
  }
}

std::shared_ptr<ir::Value> ExprBuilder::ToIRConstant(constants::Value constant) const {
  switch (constant.kind()) {
    case constants::Value::Kind::kBool:
      return std::make_shared<ir::BoolConstant>(constant.AsBool());
    case constants::Value::Kind::kInt:
      return std::make_shared<ir::IntConstant>(constant.AsInt());
    case constants::Value::Kind::kString:
      return std::make_shared<ir_ext::StringConstant>(constant.AsString());
  }
}

}  // namespace ir_builder
}  // namespace lang

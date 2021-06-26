//
//  expr_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "expr_handler.h"

#include <optional>

#include "src/lang/processors/type_checker/type_resolver.h"
#include "src/lang/representation/ast/ast_util.h"
#include "src/lang/representation/types/types_util.h"

namespace lang {
namespace type_checker {

bool ExprHandler::CheckBoolExpr(ast::Expr* expr) {
  if (!CheckValueExpr(expr)) {
    return false;
  }
  types::Type* type = types::UnderlyingOf(info()->ExprInfoOf(expr).value().type(), info_builder());
  if (type->type_kind() != types::TypeKind::kBasic ||
      !(static_cast<types::Basic*>(type)->info() & types::Basic::Info::kIsBoolean)) {
    issues().Add(issues::kExprTypeIsNotBool, expr->start(), "expression is not of type bool");
    return false;
  }
  return true;
}

bool ExprHandler::CheckIntExpr(ast::Expr* expr) {
  if (!CheckValueExpr(expr)) {
    return false;
  }
  types::Type* type = types::UnderlyingOf(info()->ExprInfoOf(expr).value().type(), info_builder());
  if (type->type_kind() != types::TypeKind::kBasic ||
      !(static_cast<types::Basic*>(type)->kind() &
        (types::Basic::Kind::kInt | types::Basic::Kind::kUntypedInt))) {
    issues().Add(issues::kExprTypeIsNotInt, expr->start(), "expression is not of type int");
    return false;
  }
  return true;
}

bool ExprHandler::CheckIntegerExpr(ast::Expr* expr) {
  if (!CheckValueExpr(expr)) {
    return false;
  }
  types::Type* type = types::UnderlyingOf(info()->ExprInfoOf(expr).value().type(), info_builder());
  if (type->type_kind() != types::TypeKind::kBasic ||
      !(static_cast<types::Basic*>(type)->info() & types::Basic::Info::kIsInteger)) {
    issues().Add(issues::kExprTypeIsNotInteger, expr->start(), "expression is not of type integer");
    return false;
  }
  return true;
}

std::vector<types::Type*> ExprHandler::CheckValueExprs(const std::vector<ast::Expr*>& exprs,
                                                       Context ctx) {
  std::vector<types::Type*> expr_types;
  expr_types.reserve(exprs.size());
  bool ok = true;
  for (ast::Expr* expr : exprs) {
    types::Type* expr_type = CheckValueExpr(expr, ctx);
    if (expr_type == nullptr) {
      ok = false;
      expr_types.clear();
    } else if (ok) {
      expr_types.push_back(expr_type);
    }
  }
  return expr_types;
}

types::Type* ExprHandler::CheckValueExpr(ast::Expr* expr, Context ctx) {
  if (!CheckExpr(expr, ctx)) {
    return nullptr;
  }
  types::ExprInfo expr_info = info()->ExprInfoOf(expr).value();
  if (!expr_info.is_value()) {
    issues().Add(issues::kExprKindIsNotValue, expr->start(), "expression is not a value");
    return nullptr;
  }
  return expr_info.type();
}

bool ExprHandler::CheckExprs(const std::vector<ast::Expr*>& exprs, Context ctx) {
  bool ok = true;
  for (ast::Expr* expr : exprs) {
    ok = CheckExpr(expr, ctx) && ok;  // Order needed to avoid short circuiting.
  }
  return ok;
}

bool ExprHandler::CheckExpr(ast::Expr* expr, Context ctx) {
  switch (expr->node_kind()) {
    case ast::NodeKind::kUnaryExpr:
      switch (static_cast<ast::UnaryExpr*>(expr)->op()) {
        case tokens::kAdd:
        case tokens::kSub:
        case tokens::kXor:
          return CheckUnaryArithmeticOrBitExpr(static_cast<ast::UnaryExpr*>(expr), ctx);
        case tokens::kNot:
          return CheckUnaryLogicExpr(static_cast<ast::UnaryExpr*>(expr), ctx);
        case tokens::kMul:
        case tokens::kRem:
        case tokens::kAnd:
          if (ctx.expect_constant_) {
            issues().Add(issues::kConstantExprContainsAddressOp, expr->start(),
                         "address operator not allowed in constant expression");
            return false;
          }
          return CheckUnaryAddressExpr(static_cast<ast::UnaryExpr*>(expr));
        default:
          throw "internal error: unexpected unary op";
      }
    case ast::NodeKind::kBinaryExpr:
      switch (static_cast<ast::BinaryExpr*>(expr)->op()) {
        case tokens::kAdd:
        case tokens::kSub:
        case tokens::kMul:
        case tokens::kQuo:
        case tokens::kRem:
        case tokens::kAnd:
        case tokens::kOr:
        case tokens::kXor:
        case tokens::kAndNot:
          return CheckBinaryArithmeticOrBitExpr(static_cast<ast::BinaryExpr*>(expr), ctx);
        case tokens::kShl:
        case tokens::kShr:
          return CheckBinaryShiftExpr(static_cast<ast::BinaryExpr*>(expr), ctx);
        case tokens::kLAnd:
        case tokens::kLOr:
          return CheckBinaryLogicExpr(static_cast<ast::BinaryExpr*>(expr), ctx);
        default:
          throw "internal error: unexpected binary op";
      }
    case ast::NodeKind::kCompareExpr:
      return CheckCompareExpr(static_cast<ast::CompareExpr*>(expr), ctx);
    case ast::NodeKind::kParenExpr:
      return CheckParenExpr(static_cast<ast::ParenExpr*>(expr), ctx);
    case ast::NodeKind::kSelectionExpr:
      return CheckSelectionExpr(static_cast<ast::SelectionExpr*>(expr), ctx);
    case ast::NodeKind::kTypeAssertExpr:
      if (ctx.expect_constant_) {
        issues().Add(issues::kConstantExprContainsTypeAssertion, expr->start(),
                     "type assertion not allowed in constant expression");
        return false;
      }
      return CheckTypeAssertExpr(static_cast<ast::TypeAssertExpr*>(expr));
    case ast::NodeKind::kIndexExpr:
      if (ctx.expect_constant_) {
        // TODO: consider supporting constant string access
        issues().Add(issues::kConstantExprContainsIndexExpr, expr->start(),
                     "index operation not allowed in constant expression");
        return false;
      }
      return CheckIndexExpr(static_cast<ast::IndexExpr*>(expr));
    case ast::NodeKind::kCallExpr:
      return CheckCallExpr(static_cast<ast::CallExpr*>(expr), ctx);
    case ast::NodeKind::kFuncLit:
      if (ctx.expect_constant_) {
        issues().Add(issues::kConstantExprContainsFuncLit, expr->start(),
                     "function literal not allowed in constant expression");
        return false;
      }
      return CheckFuncLit(static_cast<ast::FuncLit*>(expr));
    case ast::NodeKind::kCompositeLit:
      if (ctx.expect_constant_) {
        issues().Add(issues::kConstantExprContainsCompositeLit, expr->start(),
                     "composite literal not allowed in constant expression");
        return false;
      }
      return CheckCompositeLit(static_cast<ast::CompositeLit*>(expr));
    case ast::NodeKind::kBasicLit:
      return CheckBasicLit(static_cast<ast::BasicLit*>(expr));
    case ast::NodeKind::kIdent:
      return CheckIdent(static_cast<ast::Ident*>(expr), ctx);
    default:
      throw "unexpected AST expr";
  }
}

bool ExprHandler::CheckUnaryArithmeticOrBitExpr(ast::UnaryExpr* unary_expr, Context ctx) {
  std::optional<CheckBasicOperandResult> x = CheckBasicOperand(unary_expr->x(), ctx);
  if (!x.has_value()) {
    return false;
  }
  if (!(x->underlying->info() & types::Basic::Info::kIsInteger)) {
    issues().Add(issues::kUnexpectedUnaryArithemticOrBitExprOperandType, unary_expr->x()->start(),
                 "invalid operation: expected integer type");
    return false;
  }
  types::ExprInfo::Kind expr_kind = types::ExprInfo::Kind::kValue;
  std::optional<constants::Value> expr_value;
  if (x->value.has_value()) {
    constants::Value x_value = x->value.value();
    expr_value = constants::UnaryOp(unary_expr->op(), x_value);
    expr_kind = types::ExprInfo::Kind::kConstant;
  }
  info_builder().SetExprInfo(unary_expr, types::ExprInfo(expr_kind, x->type, expr_value));
  return true;
}

bool ExprHandler::CheckUnaryLogicExpr(ast::UnaryExpr* unary_expr, Context ctx) {
  std::optional<CheckBasicOperandResult> x = CheckBasicOperand(unary_expr->x(), ctx);
  if (!x.has_value()) {
    return false;
  }
  if (x->underlying->info() & types::Basic::Info::kIsBoolean) {
    issues().Add(issues::kUnexpectedUnaryLogicExprOperandType, unary_expr->x()->start(),
                 "invalid operation: expected boolean type");
    return false;
  }
  types::ExprInfo::Kind expr_kind = types::ExprInfo::Kind::kValue;
  std::optional<constants::Value> expr_value;
  if (x->value.has_value()) {
    constants::Value x_value = x->value.value();
    expr_value = constants::UnaryOp(unary_expr->op(), x_value);
    expr_kind = types::ExprInfo::Kind::kConstant;
  }
  info_builder().SetExprInfo(unary_expr, types::ExprInfo(expr_kind, x->type, expr_value));
  return true;
}

bool ExprHandler::CheckUnaryAddressExpr(ast::UnaryExpr* unary_expr) {
  if (!CheckExpr(unary_expr->x())) {
    return false;
  }
  types::ExprInfo x_info = info()->ExprInfoOf(unary_expr->x()).value();
  if (unary_expr->op() == tokens::kAnd) {
    if (!x_info.is_addressable()) {
      issues().Add(issues::kUnexpectedAddressOfExprOperandType, unary_expr->x()->start(),
                   "expression is not addressable");
      return false;
    }
    types::Pointer* pointer_type =
        info_builder().CreatePointer(types::Pointer::Kind::kStrong, x_info.type());
    info_builder().SetExprInfo(unary_expr,
                               types::ExprInfo(types::ExprInfo::Kind::kValue, pointer_type));
    return true;

  } else if (unary_expr->op() == tokens::kMul || unary_expr->op() == tokens::kRem) {
    if (x_info.type()->type_kind() != types::TypeKind::kPointer) {
      issues().Add(issues::kUnexpectedPointerDereferenceExprOperandType, unary_expr->x()->start(),
                   "invalid operation: expected pointer");
      return false;
    }
    types::Pointer* pointer = static_cast<types::Pointer*>(x_info.type());
    if (pointer->kind() == types::Pointer::Kind::kStrong && unary_expr->op() == tokens::kRem) {
      issues().Add(issues::kForbiddenWeakDereferenceOfStrongPointer, unary_expr->start(),
                   "invalid operation: can not weakly dereference strong pointer");
      return false;
    } else if (pointer->kind() == types::Pointer::Kind::kWeak && unary_expr->op() == tokens::kMul) {
      issues().Add(issues::kForbiddenStrongDereferenceOfWeakPointer, unary_expr->start(),
                   "invalid operation: can not strongly dereference weak pointer");
      return false;
    }
    info_builder().SetExprInfo(
        unary_expr, types::ExprInfo(types::ExprInfo::Kind::kVariable, pointer->element_type()));
    return true;

  } else {
    throw "internal error: unexpected unary operand";
  }
}

bool ExprHandler::CheckBinaryArithmeticOrBitExpr(ast::BinaryExpr* binary_expr, Context ctx) {
  std::optional<CheckBasicOperandResult> x = CheckBasicOperand(binary_expr->x(), ctx);
  std::optional<CheckBasicOperandResult> y = CheckBasicOperand(binary_expr->y(), ctx);
  if (!x.has_value() || !y.has_value()) {
    return false;
  }
  if (binary_expr->op() == tokens::kAdd) {
    auto check_op_type = [&](ast::Expr* op_expr, types::Basic* op_type) -> bool {
      if (op_type->info() & (types::Basic::Info::kIsString | types::Basic::Info::kIsNumeric)) {
        return true;
      }
      issues().Add(issues::kUnexpectedAddExprOperandType, op_expr->start(),
                   "invalid operation: expected string or numeric type");
      return false;
    };
    if (!check_op_type(binary_expr->x(), x->underlying) ||
        !check_op_type(binary_expr->y(), y->underlying)) {
      return false;
    }
    if ((x->underlying->info() & types::Basic::Info::kIsNumeric) !=
        (y->underlying->info() & types::Basic::Info::kIsNumeric)) {
      issues().Add(issues::kMismatchedBinaryExprTypes, binary_expr->op_start(),
                   "invalid operation: mismatched types");
      return false;
    }

  } else {
    auto check_op_type = [&](ast::Expr* op_expr, types::Basic* op_type) -> bool {
      if (op_type->info() & types::Basic::Info::kIsNumeric) {
        return true;
      }
      issues().Add(issues::kUnexpectedBinaryArithmeticOrBitExprOperandType, op_expr->start(),
                   "invalid operation: expected numeric type");
      return false;
    };
    if (!check_op_type(binary_expr->x(), x->underlying) ||
        !check_op_type(binary_expr->y(), y->underlying)) {
      return false;
    }
  }
  if (!(x->underlying->info() & types::Basic::Info::kIsUntyped) &&
      !(y->underlying->info() & types::Basic::Info::kIsUntyped) &&
      !types::IsIdentical(x->type, y->type)) {
    issues().Add(issues::kMismatchedBinaryExprTypes, binary_expr->op_start(),
                 "invalid operation: mismatched types");
    return false;
  }

  types::Type* binary_expr_type;
  if (x->type == x->underlying && x->underlying->info() & types::Basic::Info::kIsUntyped) {
    binary_expr_type = y->type;
  } else {
    binary_expr_type = x->type;
  }
  types::ExprInfo::Kind binary_expr_kind = types::ExprInfo::Kind::kValue;
  std::optional<constants::Value> binary_expr_value;
  if (x->value.has_value() && y->value.has_value()) {
    constants::Value x_value = x->value.value();
    constants::Value y_value = y->value.value();
    binary_expr_value = constants::BinaryOp(x_value, binary_expr->op(), y_value);
    binary_expr_kind = types::ExprInfo::Kind::kConstant;
  }
  info_builder().SetExprInfo(
      binary_expr, types::ExprInfo(binary_expr_kind, binary_expr_type, binary_expr_value));
  return true;
}

bool ExprHandler::CheckBinaryShiftExpr(ast::BinaryExpr* binary_expr, Context ctx) {
  std::optional<CheckBasicOperandResult> x = CheckBasicOperand(binary_expr->x(), ctx);
  std::optional<CheckBasicOperandResult> y = CheckBasicOperand(binary_expr->y(), ctx);
  if (!x.has_value() || !y.has_value()) {
    return false;
  }
  auto check_op_type = [&](ast::Expr* op_expr, types::Basic* op_type) -> bool {
    if (op_type->info() & types::Basic::Info::kIsNumeric) {
      return true;
    }
    issues().Add(issues::kUnexpectedBinaryShiftExprOperandType, op_expr->start(),
                 "invalid operation: expected numeric type");
    return false;
  };
  if (!check_op_type(binary_expr->x(), x->underlying) ||
      !check_op_type(binary_expr->y(), y->underlying)) {
    return false;
  }
  if ((y->underlying->info() & types::Basic::Info::kIsUntyped) == 0 &&
      (y->underlying->info() & types::Basic::Info::kIsUnsigned) == 0) {
    issues().Add(issues::kUnexpectedBinaryShiftExprOffsetType, binary_expr->y()->start(),
                 "invalid operation: expected untyped or unsigned numeric type");
    return false;
  }

  types::Type* expr_type = x->type;
  if (x->type == x->underlying && x->underlying->info() & types::Basic::Info::kIsUntyped) {
    expr_type = info()->basic_type(types::Basic::Kind::kInt);
  }
  types::ExprInfo::Kind expr_kind = types::ExprInfo::Kind::kValue;
  std::optional<constants::Value> expr_value;
  if (x->value.has_value() && y->value.has_value()) {
    constants::Value x_value = x->value.value();
    constants::Value y_value = y->value.value();
    if (!y_value.CanConvertToUnsigned()) {
      issues().Add(issues::kConstantBinaryShiftExprOffsetIsNegative, binary_expr->y()->start(),
                   "invalid operation: expected non-negative shift offset operand value");
      return false;
    } else {
      y_value = constants::Value(y_value.ConvertToUnsigned());
    }
    expr_value = constants::ShiftOp(x_value, binary_expr->op(), y_value);
    expr_kind = types::ExprInfo::Kind::kConstant;
  }
  info_builder().SetExprInfo(binary_expr, types::ExprInfo(expr_kind, expr_type, expr_value));
  return true;
}

bool ExprHandler::CheckBinaryLogicExpr(ast::BinaryExpr* binary_expr, Context ctx) {
  std::optional<CheckBasicOperandResult> x = CheckBasicOperand(binary_expr->x(), ctx);
  std::optional<CheckBasicOperandResult> y = CheckBasicOperand(binary_expr->y(), ctx);
  if (!x.has_value() || !y.has_value()) {
    return false;
  }
  auto check_op_type = [&](ast::Expr* op_expr, types::Basic* op_type) -> bool {
    if (op_type->info() & types::Basic::Info::kIsBoolean) {
      return true;
    }
    issues().Add(issues::kUnexpectedBinaryLogicExprOperandType, op_expr->start(),
                 "invalid operation: expected boolean type");
    return false;
  };
  if (!check_op_type(binary_expr->x(), x->underlying) ||
      !check_op_type(binary_expr->y(), y->underlying)) {
    return false;
  }
  if (!(x->underlying->info() & types::Basic::Info::kIsUntyped) &&
      !(y->underlying->info() & types::Basic::Info::kIsUntyped) &&
      !types::IsIdentical(x->type, y->type)) {
    issues().Add(issues::kMismatchedBinaryExprTypes, binary_expr->op_start(),
                 "invalid operation: mismatched types");
    return false;
  }

  types::Type* binary_expr_type;
  if (x->type == x->underlying && x->underlying->info() & types::Basic::Info::kIsUntyped) {
    binary_expr_type = y->type;
  } else {
    binary_expr_type = x->type;
  }
  std::optional<constants::Value> binary_expr_value;
  types::ExprInfo::Kind binary_expr_kind = types::ExprInfo::Kind::kValue;
  if (x->value.has_value() && y->value.has_value()) {
    constants::Value x_value = x->value.value();
    constants::Value y_value = y->value.value();
    binary_expr_value = constants::BinaryOp(x_value, binary_expr->op(), y_value);
    binary_expr_kind = types::ExprInfo::Kind::kConstant;
  }
  info_builder().SetExprInfo(
      binary_expr, types::ExprInfo(binary_expr_kind, binary_expr_type, binary_expr_value));
  return true;
}

bool ExprHandler::CheckCompareExpr(ast::CompareExpr* compare_expr, Context ctx) {
  std::vector<types::Type*> operand_types = CheckValueExprs(compare_expr->operands(), ctx);
  if (operand_types.empty()) {
    return false;
  }
  bool expr_value = true;
  types::ExprInfo::Kind expr_kind = types::ExprInfo::Kind::kConstant;
  for (size_t i = 0; i < compare_expr->compare_ops().size(); i++) {
    tokens::Token op = compare_expr->compare_ops().at(i);
    types::Type* x = operand_types.at(i);
    types::Type* y = operand_types.at(i + 1);
    switch (op) {
      case tokens::kEql:
      case tokens::kNeq:
        if (!types::IsComparable(x, y)) {
          issues().Add(issues::kCompareExprOperandTypesNotComparable,
                       compare_expr->compare_op_starts().at(i),
                       "invalid operation: types are not comparable");
          return false;
        }
        break;
      case tokens::kLss:
      case tokens::kGtr:
      case tokens::kGeq:
      case tokens::kLeq:
        if (!types::IsOrderable(x, y)) {
          issues().Add(issues::kCompareExprOperandTypesNotOrderable,
                       compare_expr->compare_op_starts().at(i),
                       "invalid operation: types are not orderable");
          return false;
        }
        break;
      default:
        throw "internal error: unexpected compare operation";
    }
    if (expr_kind != types::ExprInfo::Kind::kConstant) {
      continue;
    }
    types::ExprInfo x_info = info()->ExprInfoOf(compare_expr->operands().at(i)).value();
    types::ExprInfo y_info = info()->ExprInfoOf(compare_expr->operands().at(i + 1)).value();
    if (!x_info.is_constant() || !y_info.is_constant()) {
      expr_kind = types::ExprInfo::Kind::kValue;
      continue;
    }
    expr_value =
        expr_value && constants::Compare(x_info.constant_value(), op, y_info.constant_value());
  }
  if (expr_kind == types::ExprInfo::Kind::kConstant) {
    info_builder().SetExprInfo(compare_expr,
                               types::ExprInfo(types::ExprInfo::Kind::kConstant,
                                               info()->basic_type(types::Basic::kUntypedBool),
                                               constants::Value(expr_value)));
  } else {
    info_builder().SetExprInfo(
        compare_expr,
        types::ExprInfo(expr_kind, info()->basic_type(types::Basic::Kind::kUntypedBool)));
  }
  return true;
}

std::optional<ExprHandler::CheckBasicOperandResult> ExprHandler::CheckBasicOperand(
    ast::Expr* op_expr, Context ctx) {
  types::Type* op_type = CheckValueExpr(op_expr, ctx);
  if (op_type == nullptr) {
    return std::nullopt;
  }
  types::Type* op_underlying = types::UnderlyingOf(op_type, info_builder());
  if (op_underlying->type_kind() != types::TypeKind::kBasic) {
    issues().Add(issues::kUnexpectedBasicOperandType, op_expr->start(),
                 "invalid operation: operand does not have basic type");
    return std::nullopt;
  }
  std::optional<constants::Value> value;
  if (info()->ExprInfoOf(op_expr)->is_constant()) {
    value = info()->ExprInfoOf(op_expr)->constant_value();
  }
  return CheckBasicOperandResult{
      .type = op_type,
      .underlying = static_cast<types::Basic*>(op_underlying),
      .value = value,
  };
}

bool ExprHandler::CheckParenExpr(ast::ParenExpr* paren_expr, Context ctx) {
  if (!CheckExpr(paren_expr->x(), ctx)) {
    return false;
  }
  types::ExprInfo x_info = info()->ExprInfoOf(paren_expr->x()).value();
  info_builder().SetExprInfo(paren_expr, x_info);
  return true;
}

bool ExprHandler::CheckSelectionExpr(ast::SelectionExpr* selection_expr, Context ctx) {
  switch (CheckPackageSelectionExpr(selection_expr, ctx)) {
    case CheckSelectionExprResult::kNotApplicable:
      break;
    case CheckSelectionExprResult::kCheckFailed:
      return false;
    case CheckSelectionExprResult::kCheckSucceeded:
      return true;
  }
  if (ctx.expect_constant_) {
    issues().Add(issues::kConstantExprContainsNonPackageSelection, selection_expr->start(),
                 "selection from non-package not allowed in constant expression");
    return false;
  }

  if (!CheckExpr(selection_expr->accessed())) {
    return false;
  }
  types::ExprInfo accessed_info = info()->ExprInfoOf(selection_expr->accessed()).value();
  if (!accessed_info.is_type() && !accessed_info.is_value()) {
    issues().Add(issues::kUnexpectedSelectionAccessedExprKind, selection_expr->accessed()->start(),
                 "expression is not a type or value");
    return false;
  }
  types::Type* accessed_type = accessed_info.type();
  if (accessed_type->type_kind() == types::TypeKind::kPointer) {
    accessed_type = static_cast<types::Pointer*>(accessed_type)->element_type();
    types::Type* underlying = types::UnderlyingOf(accessed_type, info_builder());
    if (underlying->type_kind() == types::TypeKind::kInterface ||
        accessed_type->type_kind() == types::TypeKind::kTypeParameter) {
      issues().Add(issues::kForbiddenSelectionFromPointerToInterfaceOrTypeParameter,
                   selection_expr->selection()->start(),
                   "invalid operation: selection from pointer to interface or type parameter not "
                   "allowed");
      return false;
    }
  }
  if (accessed_type->type_kind() == types::TypeKind::kTypeParameter) {
    accessed_type = static_cast<types::TypeParameter*>(accessed_type)->interface();
  }
  types::InfoBuilder::TypeParamsToArgsMap type_params_to_args;
  if (accessed_type->type_kind() == types::TypeKind::kTypeInstance) {
    types::TypeInstance* type_instance = static_cast<types::TypeInstance*>(accessed_type);
    types::NamedType* instantiated_type = type_instance->instantiated_type();
    accessed_type = instantiated_type;
    for (size_t i = 0; i < type_instance->type_args().size(); i++) {
      types::TypeParameter* type_param = instantiated_type->type_parameters().at(i);
      types::Type* type_arg = type_instance->type_args().at(i);
      type_params_to_args.insert({type_param, type_arg});
    }
  }

  if (accessed_type->type_kind() == types::TypeKind::kNamedType) {
    types::NamedType* named_type = static_cast<types::NamedType*>(accessed_type);
    switch (CheckNamedTypeMethodSelectionExpr(selection_expr, named_type, type_params_to_args)) {
      case CheckSelectionExprResult::kNotApplicable:
        break;
      case CheckSelectionExprResult::kCheckFailed:
        return false;
      case CheckSelectionExprResult::kCheckSucceeded:
        return true;
    }
    accessed_type = named_type->underlying();
  }
  switch (accessed_info.kind()) {
    case types::ExprInfo::Kind::kVariable:
    case types::ExprInfo::Kind::kValue:
    case types::ExprInfo::Kind::kValueOk:
      switch (CheckStructFieldSelectionExpr(selection_expr, accessed_type, type_params_to_args)) {
        case CheckSelectionExprResult::kNotApplicable:
          break;
        case CheckSelectionExprResult::kCheckFailed:
          return false;
        case CheckSelectionExprResult::kCheckSucceeded:
          return true;
      }
      // fallthrough
    case types::ExprInfo::Kind::kType:
      switch (
          CheckInterfaceMethodSelectionExpr(selection_expr, accessed_type, type_params_to_args)) {
        case CheckSelectionExprResult::kNotApplicable:
          break;
        case CheckSelectionExprResult::kCheckFailed:
          return false;
        case CheckSelectionExprResult::kCheckSucceeded:
          return true;
      }
    default:
      break;
  }
  issues().Add(issues::kUnresolvedSelection, selection_expr->selection()->start(),
               "could not resolve selection");
  return false;
}

ExprHandler::CheckSelectionExprResult ExprHandler::CheckPackageSelectionExpr(
    ast::SelectionExpr* selection_expr, Context ctx) {
  if (selection_expr->accessed()->node_kind() != ast::NodeKind::kIdent) {
    return CheckSelectionExprResult::kNotApplicable;
  }
  ast::Ident* accessed_ident = static_cast<ast::Ident*>(selection_expr->accessed());
  types::Object* accessed_obj = info()->UseOf(accessed_ident);
  if (accessed_obj->object_kind() != types::ObjectKind::kPackageName) {
    return CheckSelectionExprResult::kNotApplicable;
  }
  if (!CheckIdent(selection_expr->selection(), ctx)) {
    return CheckSelectionExprResult::kCheckFailed;
  }
  types::ExprInfo selection_info = info()->ExprInfoOf(selection_expr->selection()).value();
  info_builder().SetExprInfo(selection_expr, selection_info);
  return CheckSelectionExprResult::kCheckSucceeded;
}

ExprHandler::CheckSelectionExprResult ExprHandler::CheckNamedTypeMethodSelectionExpr(
    ast::SelectionExpr* selection_expr, types::NamedType* named_type,
    types::InfoBuilder::TypeParamsToArgsMap type_params_to_args) {
  types::ExprInfo accessed_info = info()->ExprInfoOf(selection_expr->accessed()).value();
  std::string selection_name = selection_expr->selection()->name();
  if (!named_type->methods().contains(selection_name)) {
    return CheckSelectionExprResult::kNotApplicable;
  }
  types::Func* method = named_type->methods().at(selection_name);
  types::Signature* signature = static_cast<types::Signature*>(method->type());
  types::Type* receiver_type = nullptr;
  if (signature->has_expr_receiver()) {
    receiver_type = signature->expr_receiver()->type();
    if (receiver_type->type_kind() == types::TypeKind::kPointer) {
      receiver_type = static_cast<types::Pointer*>(receiver_type)->element_type();
    }
  } else if (signature->has_type_receiver()) {
    receiver_type = signature->type_receiver();
  }

  if (receiver_type->type_kind() == types::TypeKind::kTypeInstance) {
    types::TypeInstance* type_instance = static_cast<types::TypeInstance*>(receiver_type);
    types::InfoBuilder::TypeParamsToArgsMap method_type_params_to_args;
    method_type_params_to_args.reserve(type_params_to_args.size());
    for (size_t i = 0; i < type_instance->type_args().size(); i++) {
      types::TypeParameter* original_type_param = named_type->type_parameters().at(i);
      types::TypeParameter* method_type_param =
          static_cast<types::TypeParameter*>(type_instance->type_args().at(i));
      types::Type* type_arg = type_params_to_args.at(original_type_param);
      method_type_params_to_args.insert({method_type_param, type_arg});
    }
    type_params_to_args = method_type_params_to_args;
  }

  types::Selection::Kind selection_kind;
  if (accessed_info.is_value()) {
    signature = info_builder().InstantiateMethodSignature(signature, type_params_to_args,
                                                          /* receiver_to_arg= */ false);
    selection_kind = types::Selection::Kind::kMethodVal;
  } else if (accessed_info.is_type()) {
    bool receiver_to_arg = (signature->expr_receiver() != nullptr);
    signature =
        info_builder().InstantiateMethodSignature(signature, type_params_to_args, receiver_to_arg);
    selection_kind = types::Selection::Kind::kMethodExpr;
  } else {
    issues().Add(issues::kUnexpectedSelectionAccessedExprKind, selection_expr->accessed()->start(),
                 "expression is not a type or value");
    return CheckSelectionExprResult::kCheckFailed;
  }
  types::Selection selection(types::Selection::Kind::kMethodExpr, named_type, signature, method);
  info_builder().SetSelection(selection_expr, selection);
  info_builder().SetExprInfo(selection_expr,
                             types::ExprInfo(types::ExprInfo::Kind::kValue, signature));
  info_builder().SetUsedObject(selection_expr->selection(), method);
  return CheckSelectionExprResult::kCheckSucceeded;
}

ExprHandler::CheckSelectionExprResult ExprHandler::CheckInterfaceMethodSelectionExpr(
    ast::SelectionExpr* selection_expr, types::Type* accessed_type,
    types::InfoBuilder::TypeParamsToArgsMap type_params_to_args) {
  std::string selection_name = selection_expr->selection()->name();
  if (accessed_type->type_kind() != types::TypeKind::kInterface) {
    return CheckSelectionExprResult::kNotApplicable;
  }
  types::Interface* interface_type = static_cast<types::Interface*>(accessed_type);
  for (types::Func* method : interface_type->methods()) {
    if (method->name() != selection_name) {
      continue;
    }
    types::Signature* signature = static_cast<types::Signature*>(method->type());
    if (signature->type_receiver()) {
      types::TypeParameter* type_parameter =
          static_cast<types::TypeParameter*>(signature->type_receiver());
      type_params_to_args[type_parameter] = interface_type;
    }
    signature = info_builder().InstantiateMethodSignature(signature, type_params_to_args,
                                                          /* receiver_to_arg= */ false);
    types::Selection selection(types::Selection::Kind::kMethodVal, interface_type, method->type(),
                               method);
    info_builder().SetSelection(selection_expr, selection);
    info_builder().SetExprInfo(selection_expr,
                               types::ExprInfo(types::ExprInfo::Kind::kValue, signature));
    info_builder().SetUsedObject(selection_expr->selection(), method);
    return CheckSelectionExprResult::kCheckSucceeded;
  }
  return CheckSelectionExprResult::kNotApplicable;
}

ExprHandler::CheckSelectionExprResult ExprHandler::CheckStructFieldSelectionExpr(
    ast::SelectionExpr* selection_expr, types::Type* accessed_type,
    types::InfoBuilder::TypeParamsToArgsMap type_params_to_args) {
  std::string selection_name = selection_expr->selection()->name();
  if (accessed_type->type_kind() != types::TypeKind::kStruct) {
    return CheckSelectionExprResult::kNotApplicable;
  }
  types::Struct* struct_type = static_cast<types::Struct*>(accessed_type);
  for (types::Variable* field : struct_type->fields()) {
    if (field->name() != selection_name) {
      continue;
    }
    types::Type* field_type = field->type();
    field_type = info_builder().InstantiateType(field_type, type_params_to_args);
    types::Selection selection(types::Selection::Kind::kFieldVal, struct_type, field_type, field);
    info_builder().SetSelection(selection_expr, selection);
    info_builder().SetExprInfo(selection_expr,
                               types::ExprInfo(types::ExprInfo::Kind::kVariable, field_type));
    info_builder().SetUsedObject(selection_expr->selection(), field);
    return CheckSelectionExprResult::kCheckSucceeded;
  }
  return CheckSelectionExprResult::kNotApplicable;
}

bool ExprHandler::CheckTypeAssertExpr(ast::TypeAssertExpr* type_assert_expr) {
  if (type_assert_expr->type() == nullptr) {
    issues().Add(issues::kForbiddenBlankTypeAssertionOutsideTypeSwitch, type_assert_expr->start(),
                 "invalid operation: blank type assertion outside type switch");
    return false;
  }
  types::Type* x = CheckValueExpr(type_assert_expr->x());
  types::Type* asserted_type =
      type_resolver().type_handler().EvaluateTypeExpr(type_assert_expr->type());
  if (x == nullptr || asserted_type == nullptr) {
    return false;
  }
  if (x->type_kind() != types::TypeKind::kInterface) {
    issues().Add(issues::kUnexpectedTypeAssertionOperandType, type_assert_expr->x()->start(),
                 "invalid operation: expected interface value");
    return false;
  }
  if (!types::IsAssertableTo(x, asserted_type)) {
    issues().Add(issues::kTypeAssertionNeverPossible, type_assert_expr->start(),
                 "invalid operation: assertion always fails");
    return false;
  }

  info_builder().SetExprInfo(type_assert_expr,
                             types::ExprInfo(types::ExprInfo::Kind::kValueOk, asserted_type));
  return true;
}

bool ExprHandler::CheckIndexExpr(ast::IndexExpr* index_expr) {
  types::Type* accessed_type = CheckValueExpr(index_expr->accessed());
  if (!CheckIntExpr(index_expr->index()) || accessed_type == nullptr) {
    return false;
  }

  auto add_expected_accessed_value_issue = [&]() {
    issues().Add(issues::kUnexpectedIndexedOperandType, index_expr->start(),
                 "invalid operation: expected array, pointer to array, slice, or string, but got " +
                     accessed_type->ToString(types::StringRep::kShort));
  };
  types::Type* accessed_underlying = types::UnderlyingOf(accessed_type, info_builder());
  if (accessed_underlying->type_kind() == types::TypeKind::kPointer) {
    types::Pointer* pointer_type = static_cast<types::Pointer*>(accessed_underlying);
    if (pointer_type->element_type()->type_kind() != types::TypeKind::kArray) {
      add_expected_accessed_value_issue();
      return false;
    }
    accessed_underlying = static_cast<types::Array*>(pointer_type->element_type());
  }
  if (accessed_underlying->is_container()) {
    types::Container* container = static_cast<types::Container*>(accessed_underlying);
    types::Type* element_type = container->element_type();

    info_builder().SetExprInfo(index_expr,
                               types::ExprInfo(types::ExprInfo::Kind::kVariable, element_type));
    return true;

  } else if (accessed_underlying->type_kind() == types::TypeKind::kBasic) {
    types::Basic* accessed_basic = static_cast<types::Basic*>(accessed_underlying);
    if (!(accessed_basic->info() & types::Basic::Info::kIsString)) {
      add_expected_accessed_value_issue();
      return false;
    }

    info_builder().SetExprInfo(index_expr,
                               types::ExprInfo(types::ExprInfo::Kind::kValue,
                                               info()->basic_type(types::Basic::Kind::kByte)));
    return true;

  } else {
    add_expected_accessed_value_issue();
    return false;
  }
}

bool ExprHandler::CheckCallExpr(ast::CallExpr* call_expr, Context ctx) {
  ast::Expr* func_expr = call_expr->func();
  std::vector<ast::Expr*> type_args = call_expr->type_args();
  std::vector<ast::Expr*> args = call_expr->args();

  bool func_expr_ok = CheckExpr(func_expr);
  bool type_args_ok =
      type_args.empty() || !type_resolver().type_handler().EvaluateTypeExprs(type_args).empty();
  bool args_ok = args.empty() || !CheckValueExprs(args, ctx).empty();
  if (!func_expr_ok || !type_args_ok || !args_ok) {
    return false;
  }
  types::ExprInfo func_expr_info = info()->ExprInfoOf(func_expr).value();
  switch (func_expr_info.kind()) {
    case types::ExprInfo::Kind::kBuiltin:
      if (ctx.expect_constant_) {
        issues().Add(issues::kConstantExprContainsBuiltinCall, call_expr->start(),
                     "builtin call not allowed in constant expression");
        return false;
      }
      return CheckCallExprWithBuiltin(call_expr);
    case types::ExprInfo::Kind::kType:
      return CheckCallExprWithTypeConversion(call_expr, ctx);
    case types::ExprInfo::Kind::kVariable:
    case types::ExprInfo::Kind::kValue:
    case types::ExprInfo::Kind::kValueOk:
      if (ctx.expect_constant_) {
        issues().Add(issues::kConstantExprContainsFuncCall, call_expr->start(),
                     "function call not allowed in constant expression");
        return false;
      }
      return CheckCallExprWithFuncCall(call_expr);
    default:
      issues().Add(issues::kUnexpectedFuncExprKind, call_expr->start(),
                   "invalid operation: expression is not callable");
      return false;
  }
}

bool ExprHandler::CheckCallExprWithTypeConversion(ast::CallExpr* call_expr, Context ctx) {
  if (!call_expr->type_args().empty()) {
    // TODO: handle this in future
    issues().Add(issues::kForbiddenTypeArgumentsForTypeConversion, call_expr->start(),
                 "invalid operation: type conversion does not accept type arguments");
    return false;
  }
  if (call_expr->args().size() != 1) {
    issues().Add(issues::kWrongNumberOfArgumentsForTypeConversion, call_expr->start(),
                 "invalid operation: type conversion requires exactly one argument");
    return false;
  }
  types::ExprInfo conversion_start_info = info()->ExprInfoOf(call_expr->args().at(0)).value();
  types::ExprInfo conversion_result_info = info()->ExprInfoOf(call_expr->func()).value();
  types::Type* conversion_result_underlying =
      types::UnderlyingOf(conversion_result_info.type(), info_builder());
  if (ctx.expect_constant_ &&
      conversion_result_underlying->type_kind() != types::TypeKind::kBasic) {
    issues().Add(issues::kConstantExprContainsConversionToNonBasicType, call_expr->func()->start(),
                 "type conversion to non-basic type not allowed in constant expression");
    return false;
  }
  if (!types::IsConvertibleTo(conversion_start_info.type(), conversion_result_info.type())) {
    issues().Add(issues::kUnexpectedTypeConversionArgumentType, call_expr->start(),
                 "invalid operation: type conversion not possible");
    return false;
  }
  types::ExprInfo::Kind call_expr_kind = types::ExprInfo::Kind::kValue;
  std::optional<constants::Value> call_expr_value;
  if (conversion_start_info.is_constant() &&
      (conversion_result_underlying == nullptr ||
       conversion_result_underlying->type_kind() != types::TypeKind::kBasic)) {
    constants::Value start_value = conversion_start_info.constant_value();
    call_expr_value = types::ConvertUntypedValue(
        start_value, static_cast<types::Basic*>(conversion_result_underlying)->kind());
    call_expr_kind = types::ExprInfo::Kind::kConstant;
  }
  info_builder().SetExprInfo(
      call_expr, types::ExprInfo(call_expr_kind, conversion_result_info.type(), call_expr_value));
  return true;
}

bool ExprHandler::CheckCallExprWithBuiltin(ast::CallExpr* call_expr) {
  ast::Ident* builtin_ident = static_cast<ast::Ident*>(ast::Unparen(call_expr->func()));
  types::Builtin* builtin = static_cast<types::Builtin*>(info()->UseOf(builtin_ident));

  switch (builtin->kind()) {
    case types::Builtin::Kind::kLen: {
      if (!call_expr->type_args().empty()) {
        issues().Add(issues::kUnexpectedTypeArgumentsForLen, call_expr->start(),
                     "len does not accept type arguments");
        return false;
      }
      if (call_expr->args().size() != 1) {
        issues().Add(issues::kWrongNumberOfArgumentsForLen, call_expr->l_paren(),
                     "len expected one argument");
        return false;
      }
      ast::Expr* arg_expr = call_expr->args().at(0);
      types::Type* arg_type =
          types::UnderlyingOf(info()->ExprInfoOf(arg_expr).value().type(), info_builder());
      if ((arg_type->type_kind() != types::TypeKind::kBasic ||
           static_cast<types::Basic*>(arg_type)->kind() != types::Basic::kString) &&
          arg_type->type_kind() != types::TypeKind::kArray &&
          arg_type->type_kind() != types::TypeKind::kSlice) {
        issues().Add(issues::kUnexpectedLenArgumentType, arg_expr->start(),
                     "len expected array, slice, or string");
        return false;
      }
      info_builder().SetExprInfo(
          call_expr,
          types::ExprInfo(types::ExprInfo::Kind::kValue, info()->basic_type(types::Basic::kInt)));
      return true;
    }
    case types::Builtin::Kind::kMake: {
      if (call_expr->type_args().size() != 1) {
        issues().Add(issues::kWrongNumberOfTypeArgumentsForMake, call_expr->start(),
                     "make expected one type argument");
        return false;
      }
      if (call_expr->args().size() != 1) {
        issues().Add(issues::kWrongNumberOfArgumentsForMake, call_expr->l_paren(),
                     "make expected one argument");
        return false;
      }
      ast::Expr* slice_expr = call_expr->type_args().at(0);
      types::ExprInfo slice_expr_info = info()->ExprInfoOf(slice_expr).value();
      if (slice_expr_info.type()->type_kind() == types::TypeKind::kSlice) {
        issues().Add(issues::kUnexpectedTypeArgumentForMake, slice_expr->start(),
                     "make expected slice type argument");
        return false;
      }
      types::Slice* slice = static_cast<types::Slice*>(slice_expr_info.type());
      ast::Expr* length_expr = call_expr->args().at(0);
      types::ExprInfo length_expr_info = info()->ExprInfoOf(length_expr).value();
      if (length_expr_info.type()->type_kind() != types::TypeKind::kBasic) {
        issues().Add(issues::kUnexpectedMakeArgumentType, length_expr->start(),
                     "make expected length of type int");
        return false;
      }
      types::Basic* length_type = static_cast<types::Basic*>(length_expr_info.type());
      if (length_type->kind() != types::Basic::kInt &&
          length_type->kind() != types::Basic::kUntypedInt) {
        issues().Add(issues::kUnexpectedMakeArgumentType, length_expr->start(),
                     "make expected length of type int");
        return false;
      }
      info_builder().SetExprInfo(call_expr, types::ExprInfo(types::ExprInfo::Kind::kValue, slice));
      return true;
    }
    case types::Builtin::Kind::kNew: {
      if (call_expr->type_args().size() != 1) {
        issues().Add(issues::kWrongNumberOfTypeArgumentsForNew, call_expr->start(),
                     "new expected one type argument");
        return false;
      }
      if (!call_expr->args().empty()) {
        issues().Add(issues::kUnexpectedArgumentForNew, call_expr->l_paren(),
                     "new did not expect any arguments");
        return false;
      }
      ast::Expr* element_type_expr = call_expr->type_args().at(0);
      types::ExprInfo element_type_expr_info = info()->ExprInfoOf(element_type_expr).value();
      types::Type* element_type = element_type_expr_info.type();
      types::Pointer* pointer =
          info_builder().CreatePointer(types::Pointer::Kind::kStrong, element_type);
      info_builder().SetExprInfo(call_expr,
                                 types::ExprInfo(types::ExprInfo::Kind::kValue, pointer));
      return true;
    }
    default:
      throw "interal error: unexpected builtin kind";
  }
}

bool ExprHandler::CheckCallExprWithFuncCall(ast::CallExpr* call_expr) {
  types::ExprInfo func_expr_info = info()->ExprInfoOf(call_expr->func()).value();
  switch (func_expr_info.kind()) {
    case types::ExprInfo::Kind::kVariable:
    case types::ExprInfo::Kind::kValue:
    case types::ExprInfo::Kind::kValueOk:
      break;
    default:
      issues().Add(issues::kUnexpectedFuncCallFuncExprKind, call_expr->start(),
                   "expected type, function or function variable");
      return false;
  }
  types::Type* func_type = types::UnderlyingOf(func_expr_info.type(), info_builder());
  if (func_type->type_kind() != types::TypeKind::kSignature) {
    issues().Add(issues::kUnexpectedFuncCallFuncType, call_expr->start(),
                 "expected type, function or function variable");
    return false;
  }
  types::Signature* signature = static_cast<types::Signature*>(func_type);
  if (!signature->type_parameters().empty()) {
    signature = CheckFuncCallTypeArgs(signature, call_expr->type_args());
    if (signature == nullptr) {
      return false;
    }
  }
  CheckFuncCallArgs(signature, call_expr, call_expr->args());
  CheckFuncCallResultType(signature, call_expr);
  return true;
}

types::Signature* ExprHandler::CheckFuncCallTypeArgs(types::Signature* signature,
                                                     std::vector<ast::Expr*> type_arg_exprs) {
  size_t given_type_args = type_arg_exprs.size();
  size_t expected_type_args = signature->type_parameters().size();
  if (given_type_args != expected_type_args) {
    issues().Add(issues::kWrongNumberOfTypeArgumentsForFuncCall, type_arg_exprs.at(0)->start(),
                 "expected " + std::to_string(expected_type_args) + " type arugments");
    return nullptr;
  }
  std::unordered_map<types::TypeParameter*, types::Type*> type_params_to_args;
  for (size_t i = 0; i < expected_type_args; i++) {
    ast::Expr* type_arg_expr = type_arg_exprs.at(i);
    types::ExprInfo type_arg_expr_info = info()->ExprInfoOf(type_arg_expr).value();
    types::Type* type_arg = type_arg_expr_info.type();
    types::TypeParameter* type_param = signature->type_parameters().at(i);

    if (!types::IsAssignableTo(type_arg, type_param, info_builder())) {
      issues().Add(issues::kTypeArgumentCanNotBeUsedForFuncTypeParameter, type_arg_expr->start(),
                   "can not assign type argument to parameter");
      return nullptr;
    }
    type_params_to_args.insert({type_param, type_arg});
  }
  return info_builder().InstantiateFuncSignature(signature, type_params_to_args);
}

void ExprHandler::CheckFuncCallArgs(types::Signature* signature, ast::CallExpr* call_expr,
                                    std::vector<ast::Expr*> arg_exprs) {
  std::vector<types::Type*> arg_types;
  for (ast::Expr* arg_expr : arg_exprs) {
    arg_types.push_back(info()->ExprInfoOf(arg_expr).value().type());
  }
  if (arg_types.size() == 1 && arg_types.at(0)->type_kind() == types::TypeKind::kTuple) {
    types::Tuple* tuple = static_cast<types::Tuple*>(arg_types.at(0));
    arg_types.clear();
    for (types::Variable* var : tuple->variables()) {
      arg_types.push_back(var->type());
    }
  }
  size_t expected_args = 0;
  if (signature->parameters() != nullptr) {
    expected_args = signature->parameters()->variables().size();
  }
  if (arg_types.size() != expected_args) {
    issues().Add(issues::kWrongNumberOfArgumentsForFuncCall, call_expr->l_paren(),
                 "expected " + std::to_string(expected_args) + " arugments");
    return;
  }
  for (size_t i = 0; i < expected_args; i++) {
    types::Type* arg_type = arg_types.at(i);
    types::Type* param_type = signature->parameters()->variables().at(i)->type();

    if (!types::IsAssignableTo(arg_type, param_type, info_builder())) {
      if (arg_exprs.size() == expected_args) {
        issues().Add(issues::kUnexpectedFuncCallArgumentType, arg_exprs.at(i)->start(),
                     "can not assign argument of type " +
                         arg_type->ToString(types::StringRep::kShort) + " to parameter of type " +
                         param_type->ToString(types::StringRep::kShort));
      } else {
        issues().Add(issues::kUnexpectedFuncCallArgumentType, arg_exprs.at(0)->start(),
                     "can not assign argument of type " +
                         arg_type->ToString(types::StringRep::kShort) + " to parameter of type " +
                         param_type->ToString(types::StringRep::kShort));
        return;
      }
    }
  }
}

void ExprHandler::CheckFuncCallResultType(types::Signature* signature, ast::CallExpr* call_expr) {
  if (signature->results() == nullptr) {
    info_builder().SetExprInfo(call_expr,
                               types::ExprInfo(types::ExprInfo::Kind::kNoValue, nullptr));
    return;
  }
  if (signature->results()->variables().size() == 1) {
    info_builder().SetExprInfo(call_expr,
                               types::ExprInfo(types::ExprInfo::Kind::kValue,
                                               signature->results()->variables().at(0)->type()));
    return;
  }
  info_builder().SetExprInfo(call_expr,
                             types::ExprInfo(types::ExprInfo::Kind::kValue, signature->results()));
}

bool ExprHandler::CheckFuncLit(ast::FuncLit* func_lit) {
  ast::FuncType* func_type_expr = func_lit->type();
  ast::BlockStmt* func_body = func_lit->body();
  types::Type* func_type = type_resolver().type_handler().EvaluateTypeExpr(func_type_expr);
  if (func_type_expr == nullptr || func_type->type_kind() != types::TypeKind::kSignature) {
    return false;
  }
  types::Signature* func_signature = static_cast<types::Signature*>(func_type);
  types::Func* func = static_cast<types::Func*>(info()->ImplicitOf(func_lit));

  type_resolver().stmt_handler().CheckFuncBody(func_body, func_signature->results());

  info_builder().SetObjectType(func, func_type);
  info_builder().SetExprInfo(func_lit, types::ExprInfo(types::ExprInfo::Kind::kValue, func_type));
  return true;
}

bool ExprHandler::CheckCompositeLit(ast::CompositeLit* composite_lit) {
  types::Type* type = type_resolver().type_handler().EvaluateTypeExpr(composite_lit->type());
  if (type == nullptr) {
    return false;
  }

  // TODO: check contents of composite lit

  info_builder().SetExprInfo(composite_lit, types::ExprInfo(types::ExprInfo::Kind::kValue, type));
  return true;
}

bool ExprHandler::CheckBasicLit(ast::BasicLit* basic_lit) {
  types::Basic* type;
  constants::Value value(0);
  switch (basic_lit->kind()) {
    case tokens::kInt:
      type = info()->basic_type(types::Basic::kUntypedInt);
      value = constants::Value(std::stoll(basic_lit->value()));
      break;
    case tokens::kChar:
      // TODO: support UTF-8 and character literals
      type = info()->basic_type(types::Basic::kUntypedRune);
      value = constants::Value(int32_t(basic_lit->value().at(1)));
      break;
    case tokens::kString:
      type = info()->basic_type(types::Basic::kUntypedString);
      value = constants::Value(basic_lit->value().substr(1, basic_lit->value().length() - 2));
      break;
    default:
      throw "internal error: unexpected basic literal kind";
  }

  info_builder().SetExprInfo(basic_lit,
                             types::ExprInfo(types::ExprInfo::Kind::kConstant, type, value));
  return true;
}

bool ExprHandler::CheckIdent(ast::Ident* ident, Context ctx) {
  types::Object* object = info()->ObjectOf(ident);
  if (ctx.expect_constant_ && object->object_kind() != types::ObjectKind::kConstant) {
    issues().Add(issues::kConstantDependsOnNonConstant, ident->start(),
                 "constant can not depend on non-constant: " + ident->name());
  }
  types::ExprInfo::Kind expr_kind;
  types::Type* type = nullptr;
  std::optional<constants::Value> value;
  switch (object->object_kind()) {
    case types::ObjectKind::kTypeName:
      expr_kind = types::ExprInfo::Kind::kType;
      type = static_cast<types::TypeName*>(object)->type();
      break;
    case types::ObjectKind::kConstant:
      expr_kind = types::ExprInfo::Kind::kConstant;
      type = static_cast<types::Constant*>(object)->type();
      if (object->parent() == info()->universe() && object->name() == "iota") {
        value = constants::Value(ctx.iota_);
      } else {
        value = static_cast<types::Constant*>(object)->value();
      }
      break;
    case types::ObjectKind::kVariable:
      expr_kind = types::ExprInfo::Kind::kVariable;
      type = static_cast<types::Variable*>(object)->type();
      break;
    case types::ObjectKind::kFunc:
      expr_kind = types::ExprInfo::Kind::kVariable;
      type = static_cast<types::Func*>(object)->type();
      break;
    case types::ObjectKind::kNil:
      expr_kind = types::ExprInfo::Kind::kValue;
      type = info()->basic_type(types::Basic::kUntypedNil);
      break;
    case types::ObjectKind::kBuiltin:
      expr_kind = types::ExprInfo::Kind::kBuiltin;
      type = nullptr;
      break;
    case types::ObjectKind::kPackageName:
      issues().Add(issues::kPackageNameWithoutSelection, ident->start(),
                   "use of package name without selector");
      return false;
    default:
      throw "internal error: unexpected object type";
  }
  if (expr_kind != types::ExprInfo::Kind::kBuiltin && type == nullptr) {
    throw "internal error: expect to know type at this point";
  }

  info_builder().SetExprInfo(ident, types::ExprInfo(expr_kind, type, value));
  return true;
}

}  // namespace type_checker
}  // namespace lang

//
//  expr_handler.cc
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "expr_handler.h"

#include <optional>

#include "lang/processors/type_checker/type_resolver.h"
#include "lang/representation/ast/ast_util.h"
#include "lang/representation/types/types_util.h"

namespace lang {
namespace type_checker {

bool ExprHandler::ProcessExpr(ast::Expr* expr) { return CheckExpr(expr); }

bool ExprHandler::CheckExprs(std::vector<ast::Expr*> exprs) {
  bool ok = true;
  for (ast::Expr* expr : exprs) {
    ok = CheckExpr(expr) && ok;  // Order needed to avoid short circuiting.
  }
  return ok;
}

bool ExprHandler::CheckExpr(ast::Expr* expr) {
  switch (expr->node_kind()) {
    case ast::NodeKind::kUnaryExpr:
      switch (static_cast<ast::UnaryExpr*>(expr)->op()) {
        case tokens::kAdd:
        case tokens::kSub:
        case tokens::kXor:
          return CheckUnaryArithmeticOrBitExpr(static_cast<ast::UnaryExpr*>(expr));
        case tokens::kNot:
          return CheckUnaryLogicExpr(static_cast<ast::UnaryExpr*>(expr));
        case tokens::kMul:
        case tokens::kRem:
        case tokens::kAnd:
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
          return CheckBinaryArithmeticOrBitExpr(static_cast<ast::BinaryExpr*>(expr));
        case tokens::kShl:
        case tokens::kShr:
          return CheckBinaryShiftExpr(static_cast<ast::BinaryExpr*>(expr));
        case tokens::kLAnd:
        case tokens::kLOr:
          return CheckBinaryLogicExpr(static_cast<ast::BinaryExpr*>(expr));
        default:
          throw "internal error: unexpected binary op";
      }
    case ast::NodeKind::kCompareExpr:
      return CheckCompareExpr(static_cast<ast::CompareExpr*>(expr));
    case ast::NodeKind::kParenExpr:
      return CheckParenExpr(static_cast<ast::ParenExpr*>(expr));
    case ast::NodeKind::kSelectionExpr:
      return CheckSelectionExpr(static_cast<ast::SelectionExpr*>(expr));
    case ast::NodeKind::kTypeAssertExpr:
      return CheckTypeAssertExpr(static_cast<ast::TypeAssertExpr*>(expr));
    case ast::NodeKind::kIndexExpr:
      return CheckIndexExpr(static_cast<ast::IndexExpr*>(expr));
    case ast::NodeKind::kCallExpr:
      return CheckCallExpr(static_cast<ast::CallExpr*>(expr));
    case ast::NodeKind::kFuncLit:
      return CheckFuncLit(static_cast<ast::FuncLit*>(expr));
    case ast::NodeKind::kCompositeLit:
      return CheckCompositeLit(static_cast<ast::CompositeLit*>(expr));
    case ast::NodeKind::kBasicLit:
      return CheckBasicLit(static_cast<ast::BasicLit*>(expr));
    case ast::NodeKind::kIdent:
      return CheckIdent(static_cast<ast::Ident*>(expr));
    default:
      throw "unexpected AST expr";
  }
}

bool ExprHandler::CheckUnaryArithmeticOrBitExpr(ast::UnaryExpr* unary_expr) {
  std::optional<CheckBasicOperandResult> x = CheckBasicOperand(unary_expr->x());
  if (!x.has_value()) {
    return false;
  }
  if (!(x->underlying->info() & types::Basic::Info::kIsInteger)) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     unary_expr->x()->start(),
                                     "invalid operation: expected integer type"));
    return false;
  }
  info_builder().SetExprInfo(unary_expr, types::ExprInfo(types::ExprInfo::Kind::kValue, x->type));
  return true;
}

bool ExprHandler::CheckUnaryLogicExpr(ast::UnaryExpr* unary_expr) {
  std::optional<CheckBasicOperandResult> x = CheckBasicOperand(unary_expr->x());
  if (!x.has_value()) {
    return false;
  }
  if (x->underlying->info() & types::Basic::Info::kIsBoolean) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     unary_expr->x()->start(),
                                     "invalid operation: expected boolean type"));
    return false;
  }
  info_builder().SetExprInfo(unary_expr, types::ExprInfo(types::ExprInfo::Kind::kValue, x->type));
  return true;
}

bool ExprHandler::CheckUnaryAddressExpr(ast::UnaryExpr* unary_expr) {
  if (!CheckExpr(unary_expr->x())) {
    return false;
  }
  types::ExprInfo x_info = info()->ExprInfoOf(unary_expr->x()).value();
  if (unary_expr->op() == tokens::kAnd) {
    if (!x_info.is_addressable()) {
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       unary_expr->x()->start(), "expression is not addressable"));
      return false;
    }
    types::Pointer* pointer_type =
        info_builder().CreatePointer(types::Pointer::Kind::kStrong, x_info.type());
    info_builder().SetExprInfo(unary_expr,
                               types::ExprInfo(types::ExprInfo::Kind::kValue, pointer_type));
    return true;

  } else if (unary_expr->op() == tokens::kMul || unary_expr->op() == tokens::kRem) {
    if (x_info.type()->type_kind() != types::TypeKind::kPointer) {
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       unary_expr->x()->start(),
                                       "invalid operation: expected pointer"));
      return false;
    }
    types::Pointer* pointer = static_cast<types::Pointer*>(x_info.type());
    if (pointer->kind() == types::Pointer::Kind::kStrong && unary_expr->op() == tokens::kRem) {
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       unary_expr->start(),
                                       "invalid operation: can not weakly dereference strong "
                                       "pointer"));
      return false;
    } else if (pointer->kind() == types::Pointer::Kind::kWeak && unary_expr->op() == tokens::kMul) {
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       unary_expr->start(),
                                       "invalid operation: can not strongly dereference weak "
                                       "pointer"));
      return false;
    }
    info_builder().SetExprInfo(
        unary_expr, types::ExprInfo(types::ExprInfo::Kind::kVariable, pointer->element_type()));
    return true;

  } else {
    throw "internal error: unexpected unary operand";
  }
}

bool ExprHandler::CheckBinaryArithmeticOrBitExpr(ast::BinaryExpr* binary_expr) {
  std::optional<CheckBasicOperandResult> x = CheckBasicOperand(binary_expr->x());
  std::optional<CheckBasicOperandResult> y = CheckBasicOperand(binary_expr->y());
  if (!x.has_value() || !y.has_value()) {
    return false;
  }
  if (binary_expr->op() == tokens::kAdd) {
    auto check_op_type = [&](ast::Expr* op_expr, types::Basic* op_type) -> bool {
      if (op_type->info() & (types::Basic::Info::kIsString | types::Basic::Info::kIsNumeric)) {
        return true;
      }
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       op_expr->start(),
                                       "invalid operation: expected string or numeric type"));
      return false;
    };
    if (!check_op_type(binary_expr->x(), x->underlying) ||
        !check_op_type(binary_expr->y(), y->underlying)) {
      return false;
    }
    if ((x->underlying->info() & types::Basic::Info::kIsNumeric) !=
        (y->underlying->info() & types::Basic::Info::kIsNumeric)) {
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       binary_expr->op_start(),
                                       "invalid operation: mismatched types"));
      return false;
    }

  } else {
    auto check_op_type = [&](ast::Expr* op_expr, types::Basic* op_type) -> bool {
      if (op_type->info() & types::Basic::Info::kIsNumeric) {
        return true;
      }
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       op_expr->start(),
                                       "invalid operation: expected numeric type"));
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
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     binary_expr->op_start(),
                                     "invalid operation: mismatched types"));
    return false;
  }

  types::Type* binary_expr_type;
  if (x->type == x->underlying && x->underlying->info() & types::Basic::Info::kIsUntyped) {
    binary_expr_type = y->type;
  } else {
    binary_expr_type = x->type;
  }
  info_builder().SetExprInfo(binary_expr,
                             types::ExprInfo(types::ExprInfo::Kind::kValue, binary_expr_type));
  return true;
}

bool ExprHandler::CheckBinaryShiftExpr(ast::BinaryExpr* binary_expr) {
  std::optional<CheckBasicOperandResult> x = CheckBasicOperand(binary_expr->x());
  std::optional<CheckBasicOperandResult> y = CheckBasicOperand(binary_expr->y());
  if (!x.has_value() || !y.has_value()) {
    return false;
  }
  auto check_op_type = [&](ast::Expr* op_expr, types::Basic* op_type) -> bool {
    if (op_type->info() & types::Basic::Info::kIsNumeric) {
      return true;
    }
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     op_expr->start(), "invalid operation: expected numeric type"));
    return false;
  };
  if (!check_op_type(binary_expr->x(), x->underlying) ||
      !check_op_type(binary_expr->y(), y->underlying)) {
    return false;
  }

  types::Type* expr_type = x->type;
  if (x->type == x->underlying && x->underlying->info() & types::Basic::Info::kIsUntyped) {
    expr_type = info()->basic_type(types::Basic::Kind::kInt);
  }
  info_builder().SetExprInfo(binary_expr,
                             types::ExprInfo(types::ExprInfo::Kind::kValue, expr_type));
  return true;
}

bool ExprHandler::CheckBinaryLogicExpr(ast::BinaryExpr* binary_expr) {
  std::optional<CheckBasicOperandResult> x = CheckBasicOperand(binary_expr->x());
  std::optional<CheckBasicOperandResult> y = CheckBasicOperand(binary_expr->y());
  if (!x.has_value() || !y.has_value()) {
    return false;
  }
  auto check_op_type = [&](ast::Expr* op_expr, types::Basic* op_type) -> bool {
    if (op_type->info() & types::Basic::Info::kIsBoolean) {
      return true;
    }
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     op_expr->start(), "invalid operation: expected boolean type"));
    return false;
  };
  if (!check_op_type(binary_expr->x(), x->underlying) ||
      !check_op_type(binary_expr->y(), y->underlying)) {
    return false;
  }
  if (!(x->underlying->info() & types::Basic::Info::kIsUntyped) &&
      !(y->underlying->info() & types::Basic::Info::kIsUntyped) &&
      !types::IsIdentical(x->type, y->type)) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     binary_expr->op_start(),
                                     "invalid operation: mismatched types"));
    return false;
  }

  types::Type* binary_expr_type;
  if (x->type == x->underlying && x->underlying->info() & types::Basic::Info::kIsUntyped) {
    binary_expr_type = y->type;
  } else {
    binary_expr_type = x->type;
  }
  info_builder().SetExprInfo(binary_expr,
                             types::ExprInfo(types::ExprInfo::Kind::kValue, binary_expr_type));
  return true;
}

bool ExprHandler::CheckCompareExpr(ast::CompareExpr* compare_expr) {
  bool operands_ok = true;
  std::vector<types::ExprInfo> operand_expr_infos;
  for (ast::Expr* operand : compare_expr->operands()) {
    if (!CheckExpr(operand)) {
      operands_ok = false;
      continue;
    }
    types::ExprInfo operand_info = info()->ExprInfoOf(operand).value();
    if (!operand_info.is_value()) {
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       operand->start(), "expression is not a value"));
      return false;
    }
    if (operands_ok) {
      operand_expr_infos.push_back(operand_info);
    }
  }
  if (!operands_ok) {
    return false;
  }
  for (size_t i = 0; i < compare_expr->compare_ops().size(); i++) {
    tokens::Token op = compare_expr->compare_ops().at(i);
    types::ExprInfo x = operand_expr_infos.at(i);
    types::ExprInfo y = operand_expr_infos.at(i + 1);
    switch (op) {
      case tokens::kEql:
      case tokens::kNeq:
        if (!types::IsComparable(x.type(), y.type())) {
          issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                           compare_expr->compare_op_starts().at(i),
                                           "invalid operation: types are comparable"));
          return false;
        }
        break;
      case tokens::kLss:
      case tokens::kGtr:
      case tokens::kGeq:
      case tokens::kLeq:
        if (!types::IsOrderable(x.type(), y.type())) {
          issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                           compare_expr->compare_op_starts().at(i),
                                           "invalid operation: types are not orderable"));
          return false;
        }
        break;
      default:
        throw "internal error: unexpected compare operation";
    }
  }
  info_builder().SetExprInfo(compare_expr,
                             types::ExprInfo(types::ExprInfo::Kind::kValue,
                                             info()->basic_type(types::Basic::Kind::kBool)));
  return true;
}

std::optional<ExprHandler::CheckBasicOperandResult> ExprHandler::CheckBasicOperand(
    ast::Expr* op_expr) {
  if (!CheckExpr(op_expr)) {
    return std::nullopt;
  }
  types::ExprInfo op_info = info()->ExprInfoOf(op_expr).value();
  if (!op_info.is_value()) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     op_expr->start(), "expression is not a value"));
    return std::nullopt;
  }
  types::Type* op_type = op_info.type();
  types::Type* op_underlying = types::UnderlyingOf(op_type);
  if (op_underlying == nullptr || op_underlying->type_kind() != types::TypeKind::kBasic) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     op_expr->start(),
                                     "invalid operation: operand does not have basic type"));
    return std::nullopt;
  }
  return CheckBasicOperandResult{
      .type = op_type,
      .underlying = static_cast<types::Basic*>(op_underlying),
  };
}

bool ExprHandler::CheckParenExpr(ast::ParenExpr* paren_expr) {
  if (!CheckExpr(paren_expr->x())) {
    return false;
  }
  types::ExprInfo x_info = info()->ExprInfoOf(paren_expr->x()).value();
  info_builder().SetExprInfo(paren_expr, x_info);
  return true;
}

bool ExprHandler::CheckSelectionExpr(ast::SelectionExpr* selection_expr) {
  switch (CheckPackageSelectionExpr(selection_expr)) {
    case CheckSelectionExprResult::kNotApplicable:
      break;
    case CheckSelectionExprResult::kCheckFailed:
      return false;
    case CheckSelectionExprResult::kCheckSucceeded:
      return true;
    default:
      throw "internal error: unexpected CheckSelectionExprResult";
  }

  if (!CheckExpr(selection_expr->accessed())) {
    return false;
  }
  types::ExprInfo accessed_info = info()->ExprInfoOf(selection_expr->accessed()).value();
  if (!accessed_info.is_type() && !accessed_info.is_value()) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     selection_expr->accessed()->start(),
                                     "expression is not a type or value"));
    return false;
  }
  types::Type* accessed_type = accessed_info.type();
  if (accessed_type->type_kind() == types::TypeKind::kPointer) {
    accessed_type = static_cast<types::Pointer*>(accessed_type)->element_type();
    types::Type* underlying = types::UnderlyingOf(accessed_type);
    if ((underlying != nullptr && underlying->type_kind() == types::TypeKind::kInterface) ||
        accessed_type->type_kind() == types::TypeKind::kTypeParameter) {
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       selection_expr->selection()->start(),
                                       "invalid operation: selection from pointer to interface or "
                                       "type parameter not allowed"));
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
      default:
        throw "internal error: unexpected CheckSelectionExprResult";
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
        default:
          throw "internal error: unexpected CheckSelectionExprResult";
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
        default:
          throw "internal error: unexpected CheckSelectionExprResult";
      }
    default:
      break;
  }
  issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                   selection_expr->selection()->start(),
                                   "could not resolve selection"));
  return false;
}

ExprHandler::CheckSelectionExprResult ExprHandler::CheckPackageSelectionExpr(
    ast::SelectionExpr* selection_expr) {
  if (selection_expr->accessed()->node_kind() != ast::NodeKind::kIdent) {
    return CheckSelectionExprResult::kNotApplicable;
  }
  ast::Ident* accessed_ident = static_cast<ast::Ident*>(selection_expr->accessed());
  types::Object* accessed_obj = info()->UseOf(accessed_ident);
  if (accessed_obj->object_kind() != types::ObjectKind::kPackageName) {
    return CheckSelectionExprResult::kNotApplicable;
  }
  if (!CheckIdent(selection_expr->selection())) {
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
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     selection_expr->accessed()->start(),
                                     "expression is not a type or value"));
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
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     type_assert_expr->start(),
                                     "invalid operation: blank type assertion outside type "
                                     "switch"));
    return false;
  }
  if (!CheckExpr(type_assert_expr->x()) || !CheckExpr(type_assert_expr->type())) {
    return false;
  }
  types::ExprInfo x = info()->ExprInfoOf(type_assert_expr->x()).value();
  if (!x.is_value()) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     type_assert_expr->x()->start(), "expression is not a value"));
    return false;
  }
  if (x.type()->type_kind() != types::TypeKind::kInterface) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     type_assert_expr->x()->start(),
                                     "invalid operation: expected interface value"));
    return false;
  }
  types::ExprInfo asserted_type = info()->ExprInfoOf(type_assert_expr->type()).value();
  if (!types::IsAssertableTo(x.type(), asserted_type.type())) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     type_assert_expr->start(),
                                     "invalid operation: assertion always fails"));
    return false;
  }

  info_builder().SetExprInfo(
      type_assert_expr, types::ExprInfo(types::ExprInfo::Kind::kValueOk, asserted_type.type()));
  return true;
}

bool ExprHandler::CheckIndexExpr(ast::IndexExpr* index_expr) {
  if (!CheckExpr(index_expr->accessed()) || !CheckExpr(index_expr->index())) {
    return false;
  }

  types::ExprInfo index_info = info()->ExprInfoOf(index_expr->index()).value();
  if (!index_info.is_value()) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     index_expr->index()->start(), "expression is not a value"));
    return false;
  }
  types::Type* index_type = index_info.type();
  types::Type* index_underlying = types::UnderlyingOf(index_type);
  if (index_underlying == nullptr || index_underlying->type_kind() != types::TypeKind::kBasic ||
      !(static_cast<types::Basic*>(index_underlying)->kind() &
        (types::Basic::Kind::kInt | types::Basic::Kind::kUntypedInt))) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     index_expr->start(),
                                     "invalid operation: expected integer value"));
    return false;
  }

  auto add_expected_accessed_value_issue = [&]() {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     index_expr->start(),
                                     "invalid operation: expected array, pointer to array, slice, "
                                     "or string"));
  };
  types::ExprInfo accessed_info = info()->ExprInfoOf(index_expr->accessed()).value();
  if (!accessed_info.is_value()) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     index_expr->accessed()->start(), "expression is not a value"));
    return false;
  }
  types::Type* accessed_type = accessed_info.type();
  types::Type* accessed_underlying = types::UnderlyingOf(accessed_type);
  if (accessed_underlying == nullptr) {
    add_expected_accessed_value_issue();
    return false;
  }
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

bool ExprHandler::CheckCallExpr(ast::CallExpr* call_expr) {
  ast::Expr* func_expr = call_expr->func();

  if (!CheckExpr(func_expr) ||
      (!call_expr->type_args().empty() &&
       !type_resolver().type_handler().ProcessTypeArgs(call_expr->type_args())) ||
      !CheckExprs(call_expr->args())) {
    return false;
  }
  types::ExprInfo func_expr_info = info()->ExprInfoOf(func_expr).value();
  switch (func_expr_info.kind()) {
    case types::ExprInfo::Kind::kBuiltin:
      return CheckCallExprWithBuiltin(call_expr);
    case types::ExprInfo::Kind::kType:
      return CheckCallExprWithTypeConversion(call_expr);
    case types::ExprInfo::Kind::kVariable:
    case types::ExprInfo::Kind::kValue:
    case types::ExprInfo::Kind::kValueOk:
      return CheckCallExprWithFuncCall(call_expr);
    default:
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       call_expr->start(),
                                       "invalid operation: expression is not callable"));
      return false;
  }
}

bool ExprHandler::CheckCallExprWithTypeConversion(ast::CallExpr* call_expr) {
  if (!call_expr->type_args().empty()) {
    // TODO: handle this in future
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     call_expr->start(),
                                     "invalid operation: type conversion does not accept type "
                                     "arguments"));
    return false;
  }
  if (call_expr->args().size() != 1) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     call_expr->start(),
                                     "invalid operation: type conversion requires exactly one "
                                     "argument"));
    return false;
  }
  types::ExprInfo func_expr_info = info()->ExprInfoOf(call_expr->func()).value();
  types::ExprInfo arg_expr_info = info()->ExprInfoOf(call_expr->args().at(0)).value();
  if (!arg_expr_info.is_value()) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     call_expr->args().at(0)->start(),
                                     "expression is not a value"));
    return false;
  }
  types::Type* conversion_start_type = arg_expr_info.type();
  types::Type* conversion_result_type = func_expr_info.type();
  if (!types::IsConvertibleTo(conversion_start_type, conversion_result_type)) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     call_expr->start(),
                                     "invalid operation: type conversion not possible"));
    return false;
  }

  info_builder().SetExprInfo(
      call_expr, types::ExprInfo(types::ExprInfo::Kind::kValue, conversion_result_type));
  return true;
}

bool ExprHandler::CheckCallExprWithBuiltin(ast::CallExpr* call_expr) {
  ast::Ident* builtin_ident = static_cast<ast::Ident*>(ast::Unparen(call_expr->func()));
  types::Builtin* builtin = static_cast<types::Builtin*>(info()->UseOf(builtin_ident));

  switch (builtin->kind()) {
    case types::Builtin::Kind::kLen: {
      if (!call_expr->type_args().empty()) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         call_expr->start(), "len does not accept type arguments"));
        return false;
      }
      if (call_expr->args().size() != 1) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         call_expr->l_paren(), "len expected one argument"));
        return false;
      }
      ast::Expr* arg_expr = call_expr->args().at(0);
      types::ExprInfo arg_expr_info = info()->ExprInfoOf(arg_expr).value();
      if (!arg_expr_info.is_value()) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         arg_expr->start(), "expression is not a value"));
        return false;
      }
      types::Type* arg_type = types::UnderlyingOf(arg_expr_info.type());
      if (arg_type == nullptr ||
          ((arg_type->type_kind() != types::TypeKind::kBasic ||
            static_cast<types::Basic*>(arg_type)->kind() != types::Basic::kString) &&
           arg_type->type_kind() != types::TypeKind::kArray &&
           arg_type->type_kind() != types::TypeKind::kSlice)) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         arg_expr->start(),
                                         "len expected array, slice, or string"));
        return false;
      }
      info_builder().SetExprInfo(
          call_expr,
          types::ExprInfo(types::ExprInfo::Kind::kValue, info()->basic_type(types::Basic::kInt)));
      return true;
    }
    case types::Builtin::Kind::kMake: {
      if (call_expr->type_args().size() != 1) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         call_expr->start(), "make expected one type argument"));
        return false;
      }
      if (call_expr->args().size() != 1) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         call_expr->l_paren(), "make expected one argument"));
        return false;
      }
      ast::Expr* slice_expr = call_expr->type_args().at(0);
      types::ExprInfo slice_expr_info = info()->ExprInfoOf(slice_expr).value();
      if (slice_expr_info.type()->type_kind() == types::TypeKind::kSlice) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         slice_expr->start(), "make expected slice type argument"));
        return false;
      }
      types::Slice* slice = static_cast<types::Slice*>(slice_expr_info.type());
      ast::Expr* length_expr = call_expr->args().at(0);
      types::ExprInfo length_expr_info = info()->ExprInfoOf(length_expr).value();
      if (!length_expr_info.is_value()) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         length_expr->start(), "expression is not a value"));
        return false;
      }
      if (length_expr_info.type()->type_kind() != types::TypeKind::kBasic) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         length_expr->start(), "make expected length of type int"));
        return false;
      }
      types::Basic* length_type = static_cast<types::Basic*>(length_expr_info.type());
      if (length_type->kind() != types::Basic::kInt &&
          length_type->kind() != types::Basic::kUntypedInt) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         length_expr->start(), "make expected length of type int"));
        return false;
      }
      info_builder().SetExprInfo(call_expr, types::ExprInfo(types::ExprInfo::Kind::kValue, slice));
      return true;
    }
    case types::Builtin::Kind::kNew: {
      if (call_expr->type_args().size() != 1) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         call_expr->start(), "new expected one type argument"));
        return false;
      }
      if (!call_expr->args().empty()) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         call_expr->l_paren(), "new did not expect any arguments"));
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
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       call_expr->start(),
                                       "expected type, function or function variable"));
      return false;
  }
  types::Type* func_type = types::UnderlyingOf(func_expr_info.type());
  if (func_type == nullptr || func_type->type_kind() != types::TypeKind::kSignature) {
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     call_expr->start(),
                                     "expected type, function or function variable"));
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
    issues().push_back(issues::Issue(
        issues::Origin::TypeChecker, issues::Severity::Error, type_arg_exprs.at(0)->start(),
        "expected " + std::to_string(expected_type_args) + " type arugments"));
    return nullptr;
  }
  std::unordered_map<types::TypeParameter*, types::Type*> type_params_to_args;
  for (size_t i = 0; i < expected_type_args; i++) {
    ast::Expr* type_arg_expr = type_arg_exprs.at(i);
    types::ExprInfo type_arg_expr_info = info()->ExprInfoOf(type_arg_expr).value();
    types::Type* type_arg = type_arg_expr_info.type();
    types::TypeParameter* type_param = signature->type_parameters().at(i);

    if (!types::IsAssignableTo(type_arg, type_param)) {
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       type_arg_expr->start(),
                                       "can not assign type argument to parameter"));
      return nullptr;
    }
    type_params_to_args.insert({type_param, type_arg});
  }
  return info_builder().InstantiateFuncSignature(signature, type_params_to_args);
}

void ExprHandler::CheckFuncCallArgs(types::Signature* signature, ast::CallExpr* call_expr,
                                    std::vector<ast::Expr*> arg_exprs) {
  bool args_ok = true;
  std::vector<types::Type*> arg_types;
  for (ast::Expr* arg_expr : arg_exprs) {
    types::ExprInfo arg_expr_info = info()->ExprInfoOf(arg_expr).value();
    if (!arg_expr_info.is_value()) {
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       arg_expr->start(), "expression is not a value"));
      args_ok = false;
    }
    if (args_ok) {
      arg_types.push_back(arg_expr_info.type());
    }
  }
  if (!args_ok) {
    return;
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
    issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                     call_expr->l_paren(),
                                     "expected " + std::to_string(expected_args) + " arugments"));
    return;
  }
  for (size_t i = 0; i < expected_args; i++) {
    types::Type* arg_type = arg_types.at(i);
    types::Type* param_type = signature->parameters()->variables().at(i)->type();

    if (!types::IsAssignableTo(arg_type, param_type)) {
      if (arg_exprs.size() == expected_args) {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         arg_exprs.at(i)->start(),
                                         "can not assign argument to parameter"));
      } else {
        issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                         arg_exprs.at(0)->start(),
                                         "can not assign argument to parameter"));
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
  if (!type_resolver().type_handler().ProcessTypeExpr(func_type_expr)) {
    return false;
  }
  types::Func* func = static_cast<types::Func*>(info()->ImplicitOf(func_lit));
  types::ExprInfo func_type_expr_info = info()->ExprInfoOf(func_type_expr).value();
  types::Signature* func_type = static_cast<types::Signature*>(func_type_expr_info.type());

  type_resolver().stmt_handler().ProcessFuncBody(func_body, func_type->results());

  info_builder().SetObjectType(func, func_type);
  info_builder().SetExprInfo(func_lit, types::ExprInfo(types::ExprInfo::Kind::kValue, func_type));
  return true;
}

bool ExprHandler::CheckCompositeLit(ast::CompositeLit* composite_lit) {
  if (!type_resolver().type_handler().ProcessTypeExpr(composite_lit->type())) {
    return false;
  }
  types::ExprInfo type_expr_info = info()->ExprInfoOf(composite_lit->type()).value();
  types::Type* type = type_expr_info.type();

  // TODO: check contents of composite lit

  info_builder().SetExprInfo(composite_lit, types::ExprInfo(types::ExprInfo::Kind::kValue, type));
  return true;
}

bool ExprHandler::CheckBasicLit(ast::BasicLit* basic_lit) {
  return type_resolver().constant_handler().ProcessConstantExpr(basic_lit, /* iota= */ 0);
}

bool ExprHandler::CheckIdent(ast::Ident* ident) {
  types::Object* object = info()->ObjectOf(ident);
  types::ExprInfo::Kind expr_kind;
  types::Type* type = nullptr;
  switch (object->object_kind()) {
    case types::ObjectKind::kTypeName:
      expr_kind = types::ExprInfo::Kind::kType;
      type = static_cast<types::TypeName*>(object)->type();
      break;
    case types::ObjectKind::kConstant:
      expr_kind = types::ExprInfo::Kind::kConstant;
      type = static_cast<types::Constant*>(object)->type();
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
      issues().push_back(issues::Issue(issues::Origin::TypeChecker, issues::Severity::Error,
                                       ident->start(), "use of package name without selector"));
      return false;
    default:
      throw "internal error: unexpected object type";
  }
  if (expr_kind != types::ExprInfo::Kind::kBuiltin && type == nullptr) {
    throw "internal error: expect to know type at this point";
  }

  info_builder().SetExprInfo(ident, types::ExprInfo(expr_kind, type));
  return true;
}

}  // namespace type_checker
}  // namespace lang

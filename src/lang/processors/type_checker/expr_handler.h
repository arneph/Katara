//
//  expr_handler.h
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_expr_handler_h
#define lang_type_checker_expr_handler_h

#include <optional>
#include <vector>

#include "lang/processors/issues/issues.h"
#include "lang/processors/type_checker/base_handler.h"
#include "lang/representation/ast/ast.h"
#include "lang/representation/positions/positions.h"
#include "lang/representation/types/info.h"
#include "lang/representation/types/info_builder.h"
#include "lang/representation/types/objects.h"
#include "lang/representation/types/types.h"

namespace lang {
namespace type_checker {

class ExprHandler final : public BaseHandler {
 public:
  bool ProcessExpr(ast::Expr* expr);

 private:
  ExprHandler(class TypeResolver& type_resolver, types::InfoBuilder& info_builder,
              std::vector<issues::Issue>& issues)
      : BaseHandler(type_resolver, info_builder, issues) {}

  struct CheckBasicOperandResult {
    types::Type* type;
    types::Basic* underlying;
  };
  enum class CheckSelectionExprResult {
    kNotApplicable,
    kCheckFailed,
    kCheckSucceeded,
  };

  bool CheckExprs(std::vector<ast::Expr*> exprs);
  bool CheckExpr(ast::Expr* expr);

  bool CheckUnaryArithmeticOrBitExpr(ast::UnaryExpr* unary_expr);
  bool CheckUnaryLogicExpr(ast::UnaryExpr* unary_expr);
  bool CheckUnaryAddressExpr(ast::UnaryExpr* unary_expr);
  bool CheckBinaryArithmeticOrBitExpr(ast::BinaryExpr* binary_expr);
  bool CheckBinaryShiftExpr(ast::BinaryExpr* binary_expr);
  bool CheckBinaryLogicExpr(ast::BinaryExpr* binary_expr);
  bool CheckCompareExpr(ast::CompareExpr* compare_expr);
  std::optional<CheckBasicOperandResult> CheckBasicOperand(ast::Expr* op_expr);

  bool CheckParenExpr(ast::ParenExpr* paren_expr);

  bool CheckSelectionExpr(ast::SelectionExpr* selection_expr);
  CheckSelectionExprResult CheckPackageSelectionExpr(ast::SelectionExpr* selection_expr);
  CheckSelectionExprResult CheckNamedTypeMethodSelectionExpr(
      ast::SelectionExpr* selection_expr, types::NamedType* type,
      types::InfoBuilder::TypeParamsToArgsMap type_params_to_args);
  CheckSelectionExprResult CheckInterfaceMethodSelectionExpr(
      ast::SelectionExpr* selection_expr, types::Type* accessed_type,
      types::InfoBuilder::TypeParamsToArgsMap type_params_to_args);
  CheckSelectionExprResult CheckStructFieldSelectionExpr(
      ast::SelectionExpr* selection_expr, types::Type* accessed_type,
      types::InfoBuilder::TypeParamsToArgsMap type_params_to_args);

  bool CheckTypeAssertExpr(ast::TypeAssertExpr* type_assert_expr);
  bool CheckIndexExpr(ast::IndexExpr* index_expr);

  bool CheckCallExpr(ast::CallExpr* call_expr);
  bool CheckCallExprWithTypeConversion(ast::CallExpr* call_expr);
  bool CheckCallExprWithBuiltin(ast::CallExpr* call_expr);
  bool CheckCallExprWithFuncCall(ast::CallExpr* call_expr);
  types::Signature* CheckFuncCallTypeArgs(types::Signature* signature,
                                          std::vector<ast::Expr*> type_arg_exprs);
  void CheckFuncCallArgs(types::Signature* signature, ast::CallExpr* call_expr,
                         std::vector<ast::Expr*> arg_exprs);
  void CheckFuncCallResultType(types::Signature* signature, ast::CallExpr* call_expr);

  bool CheckFuncLit(ast::FuncLit* func_lit);
  bool CheckCompositeLit(ast::CompositeLit* composite_lit);

  bool CheckBasicLit(ast::BasicLit* basic_lit);
  bool CheckIdent(ast::Ident* ident);

  friend class TypeResolver;
};

}  // namespace type_checker
}  // namespace lang

#endif /* lang_type_checker_expr_handler_h */

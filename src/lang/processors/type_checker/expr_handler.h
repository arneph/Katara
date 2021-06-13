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

#include "src/lang/processors/issues/issues.h"
#include "src/lang/processors/type_checker/base_handler.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/positions/positions.h"
#include "src/lang/representation/types/info.h"
#include "src/lang/representation/types/info_builder.h"
#include "src/lang/representation/types/objects.h"
#include "src/lang/representation/types/types.h"

namespace lang {
namespace type_checker {

class ExprHandler final : public BaseHandler {
 public:
  struct Context {
    Context(bool expect_constant = false, int64_t iota = 0)
        : expect_constant_(expect_constant), iota_(iota) {}

    bool expect_constant_;
    int64_t iota_;
  };

  // Type checks expr and checks that the expression is a boolean value. Returns if this is
  // successful.
  bool CheckBoolExpr(ast::Expr* expr);
  // Type checks expr and checks that the expression is an int value (possibly untyped). Returns if
  // this is successful.
  bool CheckIntExpr(ast::Expr* expr);
  // Type checks expr and checks that the expression is an integer value (uint8, int64, etc).
  // Returns if this is succExprHandler::Context
  bool CheckIntegerExpr(ast::Expr* expr);

  // Type checks all exprs and checks that the expressions are values. If successful, the types of
  // exprs are returned, otherwise an empty vector.
  std::vector<types::Type*> CheckValueExprs(const std::vector<ast::Expr*>& exprs,
                                            Context ctx = Context());
  // Type checks expr and checks that the expression is a value. If successful, the type of expr is
  // returned, otherwise nullptr.
  types::Type* CheckValueExpr(ast::Expr* expr, Context ctx = Context());

  // Type checks all exprs and returns if this is successful.
  bool CheckExprs(const std::vector<ast::Expr*>& exprs, Context ctx = Context());
  // Type checks expr and returns if this is successful.
  bool CheckExpr(ast::Expr* expr, Context ctx = Context());

 private:
  struct CheckBasicOperandResult {
    types::Type* type;
    types::Basic* underlying;
    std::optional<constants::Value> value;
  };

  enum class CheckSelectionExprResult {
    kNotApplicable,
    kCheckFailed,
    kCheckSucceeded,
  };

  ExprHandler(class TypeResolver& type_resolver, types::InfoBuilder& info_builder,
              issues::IssueTracker& issues)
      : BaseHandler(type_resolver, info_builder, issues) {}

  bool CheckUnaryArithmeticOrBitExpr(ast::UnaryExpr* unary_expr, Context ctx);
  bool CheckUnaryLogicExpr(ast::UnaryExpr* unary_expr, Context ctx);
  bool CheckUnaryAddressExpr(ast::UnaryExpr* unary_expr);
  bool CheckBinaryArithmeticOrBitExpr(ast::BinaryExpr* binary_expr, Context ctx);
  bool CheckBinaryShiftExpr(ast::BinaryExpr* binary_expr, Context ctx);
  bool CheckBinaryLogicExpr(ast::BinaryExpr* binary_expr, Context ctx);
  bool CheckCompareExpr(ast::CompareExpr* compare_expr, Context ctx);
  std::optional<CheckBasicOperandResult> CheckBasicOperand(ast::Expr* op_expr, Context ctx);

  bool CheckParenExpr(ast::ParenExpr* paren_expr, Context ctx);

  bool CheckSelectionExpr(ast::SelectionExpr* selection_expr, Context ctx);
  CheckSelectionExprResult CheckPackageSelectionExpr(ast::SelectionExpr* selection_expr,
                                                     Context ctx);
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

  bool CheckCallExpr(ast::CallExpr* call_expr, Context ctx);
  bool CheckCallExprWithTypeConversion(ast::CallExpr* call_expr, Context ctx);
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
  bool CheckIdent(ast::Ident* ident, Context ctx);

  friend class TypeResolver;
};

}  // namespace type_checker
}  // namespace lang

#endif /* lang_type_checker_expr_handler_h */

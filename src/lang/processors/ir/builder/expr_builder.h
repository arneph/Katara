//
//  expr_builder.h
//  Katara
//
//  Created by Arne Philipeit on 7/18/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_builder_expr_builder_h
#define lang_ir_builder_expr_builder_h

#include <memory>
#include <vector>

#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/lang/processors/ir/builder/context.h"
#include "src/lang/processors/ir/builder/type_builder.h"
#include "src/lang/processors/ir/builder/value_builder.h"
#include "src/lang/representation/ast/nodes.h"
#include "src/lang/representation/ir_extension/instrs.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"
#include "src/lang/representation/types/info.h"

namespace lang {
namespace ir_builder {

class ExprBuilder {
 public:
  ExprBuilder(types::Info* type_info, TypeBuilder& type_builder, ValueBuilder& value_builder,
              std::unordered_map<types::Func*, ir::Func*>& funcs)
      : type_info_(type_info),
        type_builder_(type_builder),
        value_builder_(value_builder),
        funcs_(funcs) {}

  std::vector<std::shared_ptr<ir::Computed>> BuildAddressesOfExprs(std::vector<ast::Expr*> exprs,
                                                                   ASTContext& ast_ctx,
                                                                   IRContext& ir_ctx);
  std::shared_ptr<ir::Computed> BuildAddressOfExpr(ast::Expr* expr, ASTContext& ast_ctx,
                                                   IRContext& ir_ctx);

  std::vector<std::shared_ptr<ir::Value>> BuildValuesOfExprs(std::vector<ast::Expr*> exprs,
                                                             ASTContext& ast_ctx,
                                                             IRContext& ir_ctx);
  std::vector<std::shared_ptr<ir::Value>> BuildValuesOfExpr(ast::Expr* expr, ASTContext& ast_ctx,
                                                            IRContext& ir_ctx);

 private:
  // Unary expressions:
  std::shared_ptr<ir::Value> BuildValueOfUnaryExpr(ast::UnaryExpr* expr, ASTContext& ast_ctx,
                                                   IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfBoolNotExpr(ast::UnaryExpr* expr, ASTContext& ast_ctx,
                                                     IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfIntUnaryExpr(ast::UnaryExpr* expr, common::Int::UnaryOp op,
                                                      ASTContext& ast_ctx, IRContext& ir_ctx);
  std::shared_ptr<ir::Computed> BuildAddressOfUnaryMemoryExpr(ast::UnaryExpr* expr,
                                                              ASTContext& ast_ctx,
                                                              IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfUnaryMemoryExpr(ast::UnaryExpr* expr, ASTContext& ast_ctx,
                                                         IRContext& ir_ctx);

  // Binary expressions:
  std::shared_ptr<ir::Value> BuildValueOfBinaryExpr(ast::BinaryExpr* expr, ASTContext& ast_ctx,
                                                    IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfStringConcatExpr(ast::BinaryExpr* expr,
                                                          ASTContext& ast_ctx, IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfIntBinaryExpr(ast::BinaryExpr* expr,
                                                       common::Int::BinaryOp op,
                                                       ASTContext& ast_ctx, IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfIntShiftExpr(ast::BinaryExpr* expr,
                                                      common::Int::ShiftOp op, ASTContext& ast_ctx,
                                                      IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfBinaryLogicExpr(ast::BinaryExpr* expr, ASTContext& ast_ctx,
                                                         IRContext& ir_ctx);

  // Comparison expressions:
  std::shared_ptr<ir::Value> BuildValueOfCompareExpr(ast::CompareExpr* expr, ASTContext& ast_ctx,
                                                     IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfSingleCompareExpr(ast::CompareExpr* expr,
                                                           ASTContext& ast_ctx, IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfMultipleCompareExpr(ast::CompareExpr* expr,
                                                             ASTContext& ast_ctx,
                                                             IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfComparison(tokens::Token op, std::shared_ptr<ir::Value> x,
                                                    types::Type* x_type,
                                                    std::shared_ptr<ir::Value> y,
                                                    types::Type* y_type, ASTContext& ast_ctx,
                                                    IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfBoolComparison(tokens::Token tok,
                                                        std::shared_ptr<ir::Value> x,
                                                        std::shared_ptr<ir::Value> y,
                                                        IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfIntComparison(tokens::Token tok,
                                                       std::shared_ptr<ir::Value> x,
                                                       std::shared_ptr<ir::Value> y,
                                                       IRContext& ir_ctx);

  // Selection expressions:
  std::vector<std::shared_ptr<ir::Value>> BuildValuesOfSelectionExpr(ast::SelectionExpr* expr,
                                                                     ASTContext& ast_ctx,
                                                                     IRContext& ir_ctx);
  std::shared_ptr<ir::Computed> BuildAddressOfStructFieldSelectionExpr(ast::SelectionExpr* expr,
                                                                       ASTContext& ast_ctx,
                                                                       IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfStructFieldSelectionExpr(ast::SelectionExpr* expr,
                                                                  ASTContext& ast_ctx,
                                                                  IRContext& ir_ctx);

  // Type assertions:
  std::vector<std::shared_ptr<ir::Value>> BuildValuesOfTypeAssertExpr(ast::TypeAssertExpr* expr,
                                                                      ASTContext& ast_ctx,
                                                                      IRContext& ir_ctx);

  // Index expressions:
  std::shared_ptr<ir::Computed> BuildAddressOfIndexExpr(ast::IndexExpr* expr, ASTContext& ast_ctx,
                                                        IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfIndexExpr(ast::IndexExpr* expr, ASTContext& ast_ctx,
                                                   IRContext& ir_ctx);

  // Function calls:
  std::vector<std::shared_ptr<ir::Value>> BuildValuesOfCallExpr(ast::CallExpr* expr,
                                                                ASTContext& ast_ctx,
                                                                IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfCallExprWithTypeConversion(ast::CallExpr* expr,
                                                                    ASTContext& ast_ctx,
                                                                    IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValuesOfCallExprWithBuiltin(ast::CallExpr* expr,
                                                              ASTContext& ast_ctx,
                                                              IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValuesOfLenCall(ast::CallExpr* expr, ASTContext& ast_ctx,
                                                  IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValuesOfMakeCall(ast::CallExpr* expr, ASTContext& ast_ctx,
                                                   IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValuesOfNewCall(ast::CallExpr* expr, IRContext& ir_ctx);
  std::vector<std::shared_ptr<ir::Value>> BuildValuesOfCallExprWithFuncCall(ast::CallExpr* expr,
                                                                            ASTContext& ast_ctx,
                                                                            IRContext& ir_ctx);

  // Literals:
  std::shared_ptr<ir::Constant> BuildValueOfFuncLit(ast::FuncLit* expr, ASTContext& ast_ctx,
                                                    IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfCompositeLit(ast::CompositeLit* expr, ASTContext& ast_ctx,
                                                      IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfBasicLit(ast::BasicLit* basic_lit);

  // Identifiers:
  std::shared_ptr<ir::Computed> BuildAddressOfIdent(ast::Ident* ident, ASTContext& ast_ctx);
  std::shared_ptr<ir::Value> BuildValueOfIdent(ast::Ident* ident, ASTContext& ast_ctx,
                                               IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfConstant(types::Constant* constant);
  std::shared_ptr<ir::Value> BuildValueOfVariable(types::Variable* var, ASTContext& ast_ctx,
                                                  IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildValueOfFunc(types::Func* func);
  std::shared_ptr<ir::Value> BuildValueOfNil();

  types::Info* type_info_;
  TypeBuilder& type_builder_;
  ValueBuilder& value_builder_;
  std::unordered_map<types::Func*, ir::Func*>& funcs_;
};

}  // namespace ir_builder
}  // namespace lang

#endif /* lang_ir_builder_expr_builder_h */

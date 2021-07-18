//
//  ir_builder.h
//  Katara
//
//  Created by Arne Philipeit on 4/18/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_builder_h
#define lang_ir_builder_h

#include <memory>
#include <unordered_map>

#include "src/common/atomics.h"
#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/program.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/lang/processors/ir_builder/context.h"
#include "src/lang/processors/ir_builder/types_builder.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/representation/ir_extension/instrs.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"
#include "src/lang/representation/types/expr_info.h"
#include "src/lang/representation/types/info.h"
#include "src/lang/representation/types/initializer.h"
#include "src/lang/representation/types/objects.h"
#include "src/lang/representation/types/package.h"
#include "src/lang/representation/types/scope.h"
#include "src/lang/representation/types/selection.h"
#include "src/lang/representation/types/types.h"
#include "src/lang/representation/types/types_util.h"

namespace lang {
namespace ir_builder {

class IRBuilder {
 public:
  static std::unique_ptr<ir::Program> TranslateProgram(packages::Package* main_package,
                                                       types::Info* type_info);

 private:
  IRBuilder(types::Info* type_info, std::unique_ptr<ir::Program>& prog);

  void PrepareDeclsInFile(ast::File* file);
  void PrepareFuncDecl(ast::FuncDecl* func_decl);

  void BuildDeclsInFile(ast::File* file);
  void BuildFuncDecl(ast::FuncDecl* func_decl);

  void BuildPrologForFunc(types::Func* types_func, Context& ctx);
  void BuildEpilogForFunc(Context& ctx);

  void BuildStmt(ast::Stmt* stmt, Context& ctx);
  void BuildBlockStmt(ast::BlockStmt* block_stmt, Context& ctx);
  void BuildDeclStmt(ast::DeclStmt* decl_stmt, Context& ctx);
  void BuildAssignStmt(ast::AssignStmt* assign_stmt, Context& ctx);
  void BuildExprStmt(ast::ExprStmt* expr_stmt, Context& ctx);
  void BuildIncDecStmt(ast::IncDecStmt* inc_dec_stmt, Context& ctx);
  void BuildReturnStmt(ast::ReturnStmt* return_stmt, Context& ctx);
  void BuildIfStmt(ast::IfStmt* if_stmt, Context& ctx);
  void BuildExprSwitchStmt(ast::ExprSwitchStmt* expr_switch_stmt, Context& ctx);
  void BuildTypeSwitchStmt(ast::TypeSwitchStmt* type_switch_stmt, Context& ctx);
  void BuildForStmt(ast::ForStmt* for_stmt, Context& ctx);
  void BuildBranchStmt(ast::BranchStmt* branch_stmt, Context& ctx);

  void BuildVarDecl(types::Variable* var, Context& ctx);
  void BuildVarDeletion(types::Variable* var, Context& ctx);

  std::vector<std::shared_ptr<ir::Computed>> BuildAddressesOfExprs(std::vector<ast::Expr*> exprs,
                                                                   Context& ctx);
  std::shared_ptr<ir::Computed> BuildAddressOfExpr(ast::Expr* expr, Context& ctx);

  std::vector<std::shared_ptr<ir::Value>> BuildValuesOfExprs(std::vector<ast::Expr*> exprs,
                                                             Context& ctx);
  std::vector<std::shared_ptr<ir::Value>> BuildValuesOfExpr(ast::Expr* expr, Context& ctx);

  std::shared_ptr<ir::Value> BuildValueOfUnaryExpr(ast::UnaryExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfBoolNotExpr(ast::UnaryExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfIntUnaryExpr(ast::UnaryExpr* expr, common::Int::UnaryOp op,
                                                      Context& ctx);
  std::shared_ptr<ir::Computed> BuildAddressOfUnaryMemoryExpr(ast::UnaryExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfUnaryMemoryExpr(ast::UnaryExpr* expr, Context& ctx);

  std::shared_ptr<ir::Value> BuildValueOfBinaryExpr(ast::BinaryExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfStringConcatExpr(ast::BinaryExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfIntBinaryExpr(ast::BinaryExpr* expr,
                                                       common::Int::BinaryOp op, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfIntShiftExpr(ast::BinaryExpr* expr,
                                                      common::Int::ShiftOp op, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfBinaryLogicExpr(ast::BinaryExpr* expr, Context& ctx);

  std::shared_ptr<ir::Value> BuildValueOfCompareExpr(ast::CompareExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfSingleCompareExpr(ast::CompareExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfMultipleCompareExpr(ast::CompareExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfComparison(tokens::Token op, std::shared_ptr<ir::Value> x,
                                                    types::Type* x_type,
                                                    std::shared_ptr<ir::Value> y,
                                                    types::Type* y_type, Context& ctx);

  std::vector<std::shared_ptr<ir::Value>> BuildValuesOfSelectionExpr(ast::SelectionExpr* expr,
                                                                     Context& ctx);
  std::shared_ptr<ir::Computed> BuildAddressOfStructFieldSelectionExpr(ast::SelectionExpr* expr,
                                                                       Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfStructFieldSelectionExpr(ast::SelectionExpr* expr,
                                                                  Context& ctx);

  std::vector<std::shared_ptr<ir::Value>> BuildValuesOfTypeAssertExpr(ast::TypeAssertExpr* expr,
                                                                      Context& ctx);

  std::shared_ptr<ir::Computed> BuildAddressOfIndexExpr(ast::IndexExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfIndexExpr(ast::IndexExpr* expr, Context& ctx);
  std::vector<std::shared_ptr<ir::Value>> BuildValuesOfCallExpr(ast::CallExpr* expr, Context& ctx);
  std::shared_ptr<ir::Constant> BuildValueOfFuncLit(ast::FuncLit* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfCompositeLit(ast::CompositeLit* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfBasicLit(ast::BasicLit* basic_lit);

  std::shared_ptr<ir::Computed> BuildAddressOfIdent(ast::Ident* ident, Context& ctx);
  std::shared_ptr<ir::Value> BuildValueOfIdent(ast::Ident* ident, Context& ctx);

  std::shared_ptr<ir::Value> BuildValueOfConversion(std::shared_ptr<ir::Value> value,
                                                    const ir::Type* desired_type, Context& ctx);

  std::shared_ptr<ir::Value> DefaultIRValueForType(types::Type* type);
  std::shared_ptr<ir::Value> ToIRConstant(constants::Value value) const;

  types::Info* type_info_;
  TypesBuilder types_builder_;
  std::unique_ptr<ir::Program>& program_;
  std::unordered_map<types::Func*, ir::Func*> funcs_;
};

}  // namespace ir_builder
}  // namespace lang

#endif /* lang_ir_builder_h */

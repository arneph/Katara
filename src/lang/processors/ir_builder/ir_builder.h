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

#include "ir/representation/block.h"
#include "ir/representation/func.h"
#include "ir/representation/instrs.h"
#include "ir/representation/num_types.h"
#include "ir/representation/program.h"
#include "ir/representation/types.h"
#include "ir/representation/values.h"
#include "lang/processors/ir_builder/context.h"
#include "lang/processors/packages/packages.h"
#include "lang/representation/ir_extension/instrs.h"
#include "lang/representation/ir_extension/types.h"
#include "lang/representation/ir_extension/values.h"
#include "lang/representation/types/expr_info.h"
#include "lang/representation/types/info.h"
#include "lang/representation/types/initializer.h"
#include "lang/representation/types/objects.h"
#include "lang/representation/types/package.h"
#include "lang/representation/types/scope.h"
#include "lang/representation/types/selection.h"
#include "lang/representation/types/types.h"
#include "lang/representation/types/types_util.h"

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

  void AddVarMalloc(types::Variable* var, Context& ctx);
  void AddVarRetain(types::Variable* var, Context& ctx);
  void AddVarRelease(types::Variable* var, Context& ctx);

  std::vector<std::shared_ptr<ir::Value>> BuildExprs(std::vector<ast::Expr*> exprs, Context& ctx);
  std::vector<std::shared_ptr<ir::Value>> BuildExpr(ast::Expr* expr, Context& ctx);

  std::shared_ptr<ir::Value> BuildUnaryExpr(ast::UnaryExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildUnaryALExpr(ast::UnaryExpr* expr, ir::UnaryALOperation op,
                                              Context& ctx);
  std::shared_ptr<ir::Value> BuildUnaryMemoryExpr(ast::UnaryExpr* expr, Context& ctx);

  std::shared_ptr<ir::Value> BuildBinaryExpr(ast::BinaryExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildStringConcatExpr(ast::BinaryExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildBinaryALExpr(ast::BinaryExpr* expr, ir::BinaryALOperation op,
                                               Context& ctx);
  std::shared_ptr<ir::Value> BuildBinaryShiftExpr(ast::BinaryExpr* expr, ir::ShiftOperation op,
                                                  Context& ctx);
  std::shared_ptr<ir::Value> BuildBinaryLogicExpr(ast::BinaryExpr* expr, Context& ctx);

  std::shared_ptr<ir::Value> BuildCompareExpr(ast::CompareExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildSingleCompareExpr(ast::CompareExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildMultipleCompareExpr(ast::CompareExpr* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildComparison(tokens::Token op, std::shared_ptr<ir::Value> x,
                                             types::Type* x_type, std::shared_ptr<ir::Value> y,
                                             types::Type* y_type, Context& ctx);

  std::vector<std::shared_ptr<ir::Value>> BuildSelectionExpr(ast::SelectionExpr* expr,
                                                             Context& ctx);
  std::vector<std::shared_ptr<ir::Value>> BuildTypeAssertExpr(ast::TypeAssertExpr* expr,
                                                              Context& ctx);
  std::shared_ptr<ir::Value> BuildIndexExpr(ast::IndexExpr* expr, Context& ctx);
  std::vector<std::shared_ptr<ir::Value>> BuildCallExpr(ast::CallExpr* expr, Context& ctx);
  std::shared_ptr<ir::Constant> BuildFuncLit(ast::FuncLit* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildCompositeLit(ast::CompositeLit* expr, Context& ctx);
  std::shared_ptr<ir::Value> BuildBasicLit(ast::BasicLit* basic_lit);
  std::shared_ptr<ir::Value> BuildIdent(ast::Ident* ident, Context& ctx);

  std::shared_ptr<ir::Value> BuildConversion(std::shared_ptr<ir::Value> value,
                                             ir::Type* desired_type, Context& ctx);

  std::shared_ptr<ir::Value> DefaultIRValueForType(types::Type* type) const;
  std::shared_ptr<ir::Value> ToIRConstant(types::Basic* basic, constants::Value value) const;
  ir::Type* ToIRType(types::Type* type) const;

  types::Info* type_info_;

  std::unique_ptr<ir::Program>& program_;
  ir_ext::String* ir_string_type_;
  ir_ext::RefCountPointer* ir_ref_count_ptr_type_;
  std::unordered_map<types::Func*, ir::Func*> funcs_;
};

}  // namespace ir_builder
}  // namespace lang

#endif /* lang_ir_builder_h */

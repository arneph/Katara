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
#include "ir/representation/instr.h"
#include "ir/representation/num_types.h"
#include "ir/representation/program.h"
#include "ir/representation/types.h"
#include "ir/representation/values.h"
#include "lang/processors/packages/packages.h"
#include "lang/representation/ir_extension/instrs.h"
#include "lang/representation/ir_extension/values.h"
#include "lang/representation/positions/positions.h"
#include "lang/representation/types/info.h"

namespace lang {
namespace ir_builder {

class IRBuilder {
 public:
  static std::unique_ptr<ir::Program> TranslateProgram(packages::Package* main_package,
                                                       types::Info* type_info);

 private:
  class Context {
   public:
    Context(ir::Func* func) : func_(func), block_(func_->entry_block()) {}

    ir::Func* func() const { return func_; }
    ir::Block* block() const { return block_; }
    void set_block(ir::Block* block) { block_ = block; }

    std::unordered_map<types::Variable*, std::shared_ptr<ir::Value>>& var_values() {
      return var_values_;
    }

    Context SubContextForBlock(ir::Block* block) const { return Context(func_, block); }

   private:
    Context(ir::Func* func, ir::Block* block) : func_(func), block_(block) {}

    ir::Func* func_;
    ir::Block* block_;
    std::unordered_map<types::Variable*, std::shared_ptr<ir::Value>> var_values_;
  };

  IRBuilder(types::Info* type_info, std::unique_ptr<ir::Program>& prog)
      : type_info_(type_info), program_(prog) {}

  void PrepareDeclsInFile(ast::File* file);
  void PrepareFuncDecl(ast::FuncDecl* func_decl);

  void BuildDeclsInFile(ast::File* file);
  void BuildFuncDecl(ast::FuncDecl* func_decl);

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

  void BuildPhiInstrsForMerge(std::vector<Context*> input_ctxs, Context& phi_ctx);

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

  std::shared_ptr<ir::Value> ConvertToType(std::shared_ptr<ir::Value> value, ir::Type* desired_type,
                                           Context& ctx);

  std::shared_ptr<ir::Value> DefaultValueForType(types::Type* type);
  
  std::shared_ptr<ir::Value> ConstantToIRValue(types::Basic* basic, constants::Value value) const;
  ir::Type* BasicToIRType(types::Basic* basic) const;

  types::Info* type_info_;

  std::unique_ptr<ir::Program>& program_;
  std::unordered_map<types::Func*, ir::Func*> funcs_;
};

}  // namespace ir_builder
}  // namespace lang

#endif /* lang_ir_builder_h */

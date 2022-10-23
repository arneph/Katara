//
//  stmt_builder.h
//  Katara
//
//  Created by Arne Philipeit on 7/18/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_builder_stmt_builder_h
#define lang_ir_builder_stmt_builder_h

#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/instrs.h"
#include "src/ir/representation/num_types.h"
#include "src/ir/representation/types.h"
#include "src/ir/representation/values.h"
#include "src/lang/processors/ir/builder/context.h"
#include "src/lang/processors/ir/builder/expr_builder.h"
#include "src/lang/processors/ir/builder/type_builder.h"
#include "src/lang/processors/ir/builder/value_builder.h"
#include "src/lang/representation/ast/nodes.h"
#include "src/lang/representation/ir_extension/instrs.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"
#include "src/lang/representation/types/info.h"

namespace lang {
namespace ir_builder {

class StmtBuilder {
 public:
  StmtBuilder(types::Info* type_info, TypeBuilder& type_builder, ValueBuilder& value_builder,
              ExprBuilder& expr_builder)
      : type_info_(type_info),
        type_builder_(type_builder),
        value_builder_(value_builder),
        expr_builder_(expr_builder) {}

  void BuildBlockStmt(ast::BlockStmt* block_stmt, ASTContext& ast_ctx, IRContext& ir_ctx);

  void BuildVarDecl(types::Variable* var, ASTContext& ast_ctx, IRContext& ir_ctx);
  void BuildVarDeletionsForASTContext(ASTContext* ast_ctx, IRContext& ir_ctx);

 private:
  void BuildStmt(ast::Stmt* stmt, ASTContext& ast_ctx, IRContext& ir_ctx);

  // Declarations & Assignments:
  void BuildDeclStmt(ast::DeclStmt* decl_stmt, ASTContext& ast_ctx, IRContext& ir_ctx);
  void BuildAssignStmt(ast::AssignStmt* assign_stmt, ASTContext& ast_ctx, IRContext& ir_ctx);

  std::vector<std::shared_ptr<ir::Value>> BuildAssignedValuesForOpAssignment(
      tokens::Token op_assign_tok, std::vector<std::shared_ptr<ir::Computed>> lhs_addresses,
      std::vector<std::shared_ptr<ir::Value>> rhs_values, IRContext& ir_ctx);
  std::shared_ptr<ir::Value> BuildAssignedValueForOpAssignment(
      tokens::Token op_assign_tok, std::shared_ptr<ir::Computed> lhs_address,
      std::shared_ptr<ir::Value> rhs_value, IRContext& ir_ctx);

  void BuildVarDeclsForValueSpec(ast::ValueSpec* value_spec, ASTContext& ast_ctx,
                                 IRContext& ir_ctx);
  void BuildVarDeclsForAssignStmt(ast::AssignStmt* assign_stmt, ASTContext& ast_ctx,
                                  IRContext& ir_ctx);
  void BuildVarDeclForIdent(ast::Ident* ident, ASTContext& ast_ctx, IRContext& ir_ctx);

  void BuildAssignmentsForValueSpec(ast::ValueSpec* value_spec, ASTContext& ast_ctx,
                                    IRContext& ir_ctx);
  void BuildAssignments(std::vector<std::shared_ptr<ir::Computed>> lhs_addresses,
                        std::vector<std::shared_ptr<ir::Value>> rhs_values, IRContext& ir_ctx);
  void BuildAssignment(std::shared_ptr<ir::Computed> lhs_address,
                       std::shared_ptr<ir::Value> rhs_value, IRContext& ir_ctx);

  // Scope cleanup:
  void BuildVarDeletionsForASTContextAndAllParents(ASTContext* ast_ctx, IRContext& ir_ctx);
  void BuildVarDeletionsForASTContextsUntilParent(ASTContext* innermost_ast_ctx,
                                                  ASTContext* outermost_ast_ctx, IRContext& ir_ctx);

  // Expressions:
  void BuildExprStmt(ast::ExprStmt* expr_stmt, ASTContext& ast_ctx, IRContext& ir_ctx);
  void BuildIncDecStmt(ast::IncDecStmt* inc_dec_stmt, ASTContext& ast_ctx, IRContext& ir_ctx);

  // Control flow:
  void BuildIfStmt(ast::IfStmt* if_stmt, ASTContext& ast_ctx, IRContext& ir_ctx);
  void BuildExprSwitchStmt(std::string label, ast::ExprSwitchStmt* expr_switch_stmt,
                           ASTContext& ast_ctx, IRContext& ir_ctx);
  void BuildTypeSwitchStmt(std::string label, ast::TypeSwitchStmt* type_switch_stmt,
                           ASTContext& ast_ctx, IRContext& ir_ctx);
  void BuildForStmt(std::string label, ast::ForStmt* for_stmt, ASTContext& ast_ctx,
                    IRContext& ir_ctx);
  void BuildBranchStmt(ast::BranchStmt* branch_stmt, ASTContext& ast_ctx, IRContext& ir_ctx);
  void BuildReturnStmt(ast::ReturnStmt* return_stmt, ASTContext& ast_ctx, IRContext& ir_ctx);

  types::Info* type_info_;
  TypeBuilder& type_builder_;
  ValueBuilder& value_builder_;
  ExprBuilder& expr_builder_;
};

}  // namespace ir_builder
}  // namespace lang

#endif /* lang_ir_builder_stmt_builder_h */

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
#include "src/lang/processors/ir_builder/context.h"
#include "src/lang/processors/ir_builder/expr_builder.h"
#include "src/lang/processors/ir_builder/type_builder.h"
#include "src/lang/representation/ast/nodes.h"
#include "src/lang/representation/ir_extension/instrs.h"
#include "src/lang/representation/ir_extension/types.h"
#include "src/lang/representation/ir_extension/values.h"
#include "src/lang/representation/types/info.h"

namespace lang {
namespace ir_builder {

class StmtBuilder {
 public:
  StmtBuilder(types::Info* type_info, TypeBuilder& type_builder, ExprBuilder& expr_builder)
      : type_info_(type_info), type_builder_(type_builder), expr_builder_(expr_builder) {}

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

 private:
  types::Info* type_info_;
  TypeBuilder& type_builder_;
  ExprBuilder& expr_builder_;
};

}  // namespace ir_builder
}  // namespace lang

#endif /* lang_ir_builder_stmt_builder_h */

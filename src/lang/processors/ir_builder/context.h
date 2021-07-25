//
//  context.h
//  Katara
//
//  Created by Arne Philipeit on 5/15/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_builder_context_h
#define lang_ir_builder_context_h

#include <memory>
#include <utility>
#include <vector>

#include "src/ir/representation/block.h"
#include "src/ir/representation/func.h"
#include "src/ir/representation/values.h"
#include "src/lang/representation/types/objects.h"

namespace lang {
namespace ir_builder {

class ASTContext {
 public:
  ASTContext(ast::BlockStmt* block) : block_(block) {}

  ASTContext* parent() const { return parent_; }
  ast::BlockStmt* block() const { return block_; }

  const std::vector<std::pair<types::Variable*, std::shared_ptr<ir::Computed>>>& var_addresses()
      const {
    return var_addresses_;
  }

  std::shared_ptr<ir::Computed> LookupAddressOfVar(types::Variable* var) const;
  void AddAddressOfVar(types::Variable* var, std::shared_ptr<ir::Computed> address);
  void ClearVarAdresses() { var_addresses_.clear(); }

  ASTContext ChildContextFor(ast::BlockStmt* block);

 private:
  ASTContext(ASTContext* parent, ast::BlockStmt* block) : parent_(parent), block_(block) {}

  ASTContext* parent_ = nullptr;
  ast::BlockStmt* block_;
  std::vector<std::pair<types::Variable*, std::shared_ptr<ir::Computed>>> var_addresses_;
};

class IRContext {
 public:
  IRContext(ir::Func* func, ir::Block* block) : func_(func), block_(block) {}

  ir::Func* func() const { return func_; }
  ir::Block* block() const { return block_; }
  void set_block(ir::Block* ir_block) { block_ = ir_block; }

  IRContext ChildContextFor(ir::Block* block) const;

 private:
  IRContext(const IRContext* parent, ir::Func* func, ir::Block* block)
      : parent_(parent), func_(func), block_(block) {}

  const IRContext* parent_ = nullptr;
  ir::Func* func_;
  ir::Block* block_;
};

}  // namespace ir_builder
}  // namespace lang

#endif /* lang_ir_builder_context_h */

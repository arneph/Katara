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
  struct BranchLookupResult {
    ir::block_num_t destination;
    ASTContext* defining_ctx;
  };

  ASTContext() {}

  ASTContext* parent() const { return parent_; }

  const std::vector<std::pair<types::Variable*, std::shared_ptr<ir::Computed>>>& var_addresses()
      const {
    return var_addresses_;
  }

  std::shared_ptr<ir::Computed> LookupAddressOfVar(types::Variable* var) const;
  void AddAddressOfVar(types::Variable* var, std::shared_ptr<ir::Computed> address);

  BranchLookupResult LookupFallthrough();
  BranchLookupResult LookupContinue();
  BranchLookupResult LookupBreak();
  BranchLookupResult LookupContinueWithLabel(std::string label);
  BranchLookupResult LookupBreakWithLabel(std::string label);

  ASTContext ChildContext();
  ASTContext ChildContextForLoop(std::string label, ir::block_num_t continue_block,
                                 ir::block_num_t break_block);

 private:
  ASTContext(ASTContext* parent) : parent_(parent) {}
  ASTContext(ASTContext* parent, std::string label, ir::block_num_t fallthrough_block,
             ir::block_num_t continue_block, ir::block_num_t break_block)
      : parent_(parent),
        label_(label),
        fallthrough_block_(fallthrough_block),
        continue_block_(continue_block),
        break_block_(break_block) {}

  ASTContext* parent_ = nullptr;

  std::vector<std::pair<types::Variable*, std::shared_ptr<ir::Computed>>> var_addresses_;

  std::string label_;
  ir::block_num_t fallthrough_block_ = ir::kNoBlockNum;
  ir::block_num_t continue_block_ = ir::kNoBlockNum;
  ir::block_num_t break_block_ = ir::kNoBlockNum;
};

class IRContext {
 public:
  IRContext(ir::Func* func, ir::Block* block) : func_(func), block_(block) {}

  ir::Func* func() const { return func_; }
  ir::Block* block() const { return block_; }
  void set_block(ir::Block* ir_block) { block_ = ir_block; }

  IRContext ChildContextFor(ir::Block* block) const;

 private:
  ir::Func* func_;
  ir::Block* block_;
};

}  // namespace ir_builder
}  // namespace lang

#endif /* lang_ir_builder_context_h */

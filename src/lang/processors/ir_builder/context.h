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
#include <unordered_map>

#include "ir/representation/block.h"
#include "ir/representation/func.h"
#include "ir/representation/values.h"
#include "lang/representation/types/objects.h"

namespace lang {
namespace ir_builder {

class Context {
 public:
  Context(ir::Func* func) : parent_ctx_(nullptr), func_(func), block_(func_->entry_block()) {}

  const Context* parent_ctx() const { return parent_ctx_; }

  ir::Func* func() const { return func_; }
  ir::Block* block() const { return block_; }
  void set_block(ir::Block* block) { block_ = block; }

  const std::unordered_map<types::Variable*, std::shared_ptr<ir::Value>>& var_addresses() const {
    return var_addresses_;
  }

  std::shared_ptr<ir::Value> LookupAddressOfVar(types::Variable* var) const;
  void AddAddressOfVar(types::Variable* var, std::shared_ptr<ir::Value> address);

  Context SubContextForBlock(ir::Block* block) const { return Context(func_, block, this); }

 private:
  Context(ir::Func* func, ir::Block* block, const Context* parent_ctx)
      : parent_ctx_(parent_ctx), func_(func), block_(block) {}

  const Context* parent_ctx_;
  ir::Func* func_;
  ir::Block* block_;
  std::unordered_map<types::Variable*, std::shared_ptr<ir::Value>> var_addresses_;
};

}  // namespace ir_builder
}  // namespace lang

#endif /* lang_ir_builder_context_h */

//
//  context.cc
//  Katara
//
//  Created by Arne Philipeit on 5/15/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "context.h"

#include "src/common/logging.h"

namespace lang {
namespace ir_builder {

std::shared_ptr<ir::Computed> ASTContext::LookupAddressOfVar(types::Variable* requested_var) const {
  for (auto& [var, address] : var_addresses_) {
    if (var == requested_var) {
      return address;
    }
  }
  if (parent_ != nullptr) {
    return parent_->LookupAddressOfVar(requested_var);
  } else {
    return nullptr;
  }
}

void ASTContext::AddAddressOfVar(types::Variable* var, std::shared_ptr<ir::Computed> address) {
  if (LookupAddressOfVar(var) != nullptr) {
    common::fail("attempted to add var address twice");
  }
  var_addresses_.emplace_back(var, address);
}

ASTContext::BranchLookupResult ASTContext::LookupFallthrough() {
  return BranchLookupResult{fallthrough_block_, this};
}

ASTContext::BranchLookupResult ASTContext::LookupContinue() {
  if (continue_block_ != ir::kNoBlockNum) {
    return BranchLookupResult{continue_block_, this};
  } else {
    return parent_->LookupContinue();
  }
}

ASTContext::BranchLookupResult ASTContext::LookupBreak() {
  if (break_block_ != ir::kNoBlockNum) {
    return BranchLookupResult{break_block_, this};
  } else {
    return parent_->LookupBreak();
  }
}

ASTContext::BranchLookupResult ASTContext::LookupContinueWithLabel(std::string label) {
  if (continue_block_ != ir::kNoBlockNum && label_ == label) {
    return BranchLookupResult{continue_block_, this};
  } else {
    return parent_->LookupContinueWithLabel(label);
  }
}

ASTContext::BranchLookupResult ASTContext::LookupBreakWithLabel(std::string label) {
  if (break_block_ != ir::kNoBlockNum && label_ == label) {
    return BranchLookupResult{break_block_, this};
  } else {
    return parent_->LookupBreakWithLabel(label);
  }
}

ASTContext ASTContext::ChildContext() { return ASTContext(this); }
ASTContext ASTContext::ChildContextForLoop(std::string label, ir::block_num_t continue_block,
                                           ir::block_num_t break_block) {
  return ASTContext(this, label, ir::kNoBlockNum, continue_block, break_block);
}

IRContext IRContext::ChildContextFor(ir::Block* block) const { return IRContext(func_, block); }

}  // namespace ir_builder
}  // namespace lang

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

ASTContext ASTContext::ChildContextFor(ast::BlockStmt* block) { return ASTContext(this, block); }

IRContext IRContext::ChildContextFor(ir::Block* block) const {
  return IRContext(this, func_, block);
}

}  // namespace ir_builder
}  // namespace lang

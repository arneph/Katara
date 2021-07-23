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

std::shared_ptr<ir::Computed> Context::LookupAddressOfVar(types::Variable* var) const {
  if (var_addresses_.contains(var)) {
    return var_addresses_.at(var);
  } else if (parent_ctx_ != nullptr) {
    return parent_ctx_->LookupAddressOfVar(var);
  } else {
    return nullptr;
  }
}

void Context::AddAddressOfVar(types::Variable* var, std::shared_ptr<ir::Computed> address) {
  if (LookupAddressOfVar(var) != nullptr) {
    common::fail("attempted to a dd var address twice");
  }
  var_addresses_.insert({var, address});
}

}  // namespace ir_builder
}  // namespace lang

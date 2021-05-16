//
//  context.cc
//  Katara
//
//  Created by Arne Philipeit on 5/15/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "context.h"

namespace lang {
namespace ir_builder {

std::shared_ptr<ir::Value> Context::LookupAddressOfVar(types::Variable* var) const {
  if (var_addresses_.contains(var)) {
    return var_addresses_.at(var);
  } else if (parent_ctx_ != nullptr) {
    return parent_ctx_->LookupAddressOfVar(var);
  } else {
    return nullptr;
  }
}

void Context::AddAddressOfVar(types::Variable* var, std::shared_ptr<ir::Value> address) {
  if (LookupAddressOfVar(var) != nullptr) {
    throw "attempted to a dd var address twice";
  }
  var_addresses_.insert({var, address});
}

}  // namespace ir_builder
}  // namespace lang

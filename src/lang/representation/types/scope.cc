//
//  scope.cc
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "scope.h"

namespace lang {
namespace types {

Object* Scope::Lookup(std::string name) const {
  auto it = named_objects_.find(name);
  if (it != named_objects_.end()) {
    return it->second;
  }
  if (parent_) {
    return parent_->Lookup(name);
  }
  return nullptr;
}

Object* Scope::Lookup(std::string name, const Scope*& scope) const {
  auto it = named_objects_.find(name);
  if (it != named_objects_.end()) {
    scope = this;
    return it->second;
  }
  if (parent_) {
    return parent_->Lookup(name, scope);
  }
  return nullptr;
}

}  // namespace types
}  // namespace lang

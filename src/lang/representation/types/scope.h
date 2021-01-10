//
//  scope.h
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_scope_h
#define lang_types_scope_h

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "lang/representation/types/objects.h"

namespace lang {
namespace types {

class Scope {
 public:
  Scope* parent() const { return parent_; }
  const std::vector<Scope*>& children() const { return children_; }
  const std::unordered_map<std::string, Object*>& named_objects() const { return named_objects_; }
  const std::unordered_set<Object*>& unnamed_objects() const { return unnamed_objects_; }

  Object* Lookup(std::string name) const;
  Object* Lookup(std::string name, const Scope*& defining_scope) const;

 private:
  Scope() {}

  Scope* parent_;
  std::vector<Scope*> children_;
  std::unordered_map<std::string, Object*> named_objects_;
  std::unordered_set<Object*> unnamed_objects_;

  friend class InfoBuilder;
};

}  // namespace types
}  // namespace lang

#endif /* lang_types_scope_h */

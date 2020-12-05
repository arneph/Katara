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
    ~Scope() {}
    
    Scope * parent() const;
    const std::vector<Scope *>& children() const;
    const std::unordered_map<std::string, Object *>& named_objects() const;
    const std::unordered_set<Object *>& unnamed_objects() const;
    
    Object * Lookup(std::string name) const;
    Object * Lookup(std::string name, const Scope*& defining_scope) const;
    
private:
    Scope() {}
    
    Scope *parent_;
    std::vector<Scope *> children_;
    std::unordered_map<std::string, Object *> named_objects_;
    std::unordered_set<Object *> unnamed_objects_;
    
    friend class InfoBuilder;
};

}
}

#endif /* lang_types_scope_h */

//
//  package.h
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_package_h
#define lang_types_package_h

#include <string>
#include <unordered_set>

#include "lang/representation/types/scope.h"

namespace lang {
namespace types {

class Package {
public:
    std::string path() const { return path_; }
    std::string name() const { return name_; }
    Scope *scope() const { return scope_; }
    const std::unordered_set<Package *>& imports() const { return imports_; }
    
private:
    Package() {}
    
    std::string path_;
    std::string name_;
    Scope *scope_;
    std::unordered_set<Package *> imports_;
    
    friend class InfoBuilder;
};

}
}

#endif /* lang_types_package_h */

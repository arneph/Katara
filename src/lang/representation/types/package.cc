//
//  package.cc
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "package.h"

namespace lang {
namespace types {

std::string Package::path() const {
    return path_;
}

std::string Package::name() const {
    return name_;
}

Scope * Package::scope() const {
    return scope_;
}

const std::unordered_set<Package *>& Package::imports() const {
    return imports_;
}

}
}

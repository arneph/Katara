//
//  initializer.cc
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "initializer.h"

namespace lang {
namespace types {

const std::vector<Variable *>& Initializer::lhs() const {
    return lhs_;
}

ast::Expr * Initializer::rhs() const {
    return rhs_;
}

}
}

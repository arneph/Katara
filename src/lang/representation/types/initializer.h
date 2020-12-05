//
//  initializer.h
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_initializer_h
#define lang_types_initializer_h

#include <vector>

#include "lang/representation/ast/ast.h"
#include "lang/representation/types/objects.h"

namespace lang {
namespace types {

class Initializer {
public:
    ~Initializer() {}
    
    const std::vector<Variable *>& lhs() const;
    ast::Expr * rhs() const;
    
private:
    Initializer() {}
    
    std::vector<Variable *> lhs_;
    ast::Expr *rhs_;
    
    friend class InfoBuilder;
};

}
}

#endif /* lang_types_initializer_h */

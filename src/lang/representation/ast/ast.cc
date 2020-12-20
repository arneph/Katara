//
//  ast.cc
//  Katara
//
//  Created by Arne Philipeit on 12/13/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "ast.h"

#include "lang/representation/ast/ast_builder.h"

namespace lang {
namespace ast {

ASTBuilder AST::builder() {
    return ASTBuilder(this);
}

}
}

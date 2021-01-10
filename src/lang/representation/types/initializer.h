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
  Initializer(std::vector<Variable*> lhs, ast::Expr* rhs) : lhs_(lhs), rhs_(rhs) {}

  const std::vector<Variable*>& lhs() const { return lhs_; }
  ast::Expr* rhs() const { return rhs_; }

 private:
  std::vector<Variable*> lhs_;
  ast::Expr* rhs_;
};

}  // namespace types
}  // namespace lang

#endif /* lang_types_initializer_h */

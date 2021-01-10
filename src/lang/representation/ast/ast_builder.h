//
//  ast_builder.h
//  Katara
//
//  Created by Arne Philipeit on 12/13/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_ast_ast_builder_h
#define lang_ast_ast_builder_h

#include <memory>

#include "lang/representation/ast/ast.h"
#include "lang/representation/ast/nodes.h"

namespace lang {
namespace ast {

class ASTBuilder {
 public:
  Package* CreatePackage(std::string name, std::map<std::string, File*> files);

  template <class T, class... Args>
  T* Create(Args&&... args) {
    return static_cast<T*>(
        ast_->node_unique_ptrs_.emplace_back(std::unique_ptr<T>(new T(std::forward<Args>(args)...)))
            .get());
  }

 private:
  ASTBuilder(AST* ast) : ast_(ast) {}

  AST* ast_;

  friend AST;
};

}  // namespace ast
}  // namespace lang

#endif /* lang_ast_ast_builder_h */

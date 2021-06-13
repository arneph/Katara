//
//  ast.h
//  Katara
//
//  Created by Arne Philipeit on 12/13/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_ast_ast_h
#define lang_ast_ast_h

#include <memory>
#include <vector>

#include "src/lang/representation/ast/nodes.h"

namespace lang {
namespace ast {

class Package {
 public:
  Package(std::string name, std::map<std::string, File*> files) : name_(name), files_(files) {}

  std::string name() const { return name_; }
  std::map<std::string, File*> files() const { return files_; }

 private:
  std::string name_;
  std::map<std::string, File*> files_;
};

class ASTBuilder;

class AST {
 public:
  std::vector<Package*> packages() const { return packages_; }

  ASTBuilder builder();

 private:
  std::vector<std::unique_ptr<Package>> package_unique_ptrs_;
  std::vector<std::unique_ptr<Node>> node_unique_ptrs_;

  std::vector<Package*> packages_;

  friend ASTBuilder;
};

}  // namespace ast
}  // namespace lang

#endif /* lang_ast_ast_h */

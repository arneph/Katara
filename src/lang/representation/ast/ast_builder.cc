//
//  ast_builder.cc
//  Katara
//
//  Created by Arne Philipeit on 12/13/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "ast_builder.h"

namespace lang {
namespace ast {

Package* ASTBuilder::CreatePackage(std::string name, std::map<std::string, File*> files) {
  std::unique_ptr<Package> package = std::make_unique<Package>(name, files);
  Package* package_ptr = package.get();
  ast_->package_unique_ptrs_.push_back(std::move(package));
  ast_->packages_.push_back(package_ptr);
  return package_ptr;
}

}  // namespace ast
}  // namespace lang

//
//  type_resolver.h
//  Katara
//
//  Created by Arne Philipeit on 1/9/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_type_resolver_h
#define lang_type_checker_type_resolver_h

#include "lang/processors/type_checker/decl_handler.h"
#include "lang/processors/type_checker/expr_handler.h"
#include "lang/processors/type_checker/stmt_handler.h"
#include "lang/processors/type_checker/type_handler.h"

namespace lang {
namespace type_checker {

class TypeResolver {
 public:
  TypeResolver(types::InfoBuilder& info_builder, issues::IssueTracker& issues)
      : type_handler_(*this, info_builder, issues),
        expr_handler_(*this, info_builder, issues),
        stmt_handler_(*this, info_builder, issues),
        decl_handler_(*this, info_builder, issues) {}

  TypeHandler& type_handler() { return type_handler_; }
  ExprHandler& expr_handler() { return expr_handler_; }
  StmtHandler& stmt_handler() { return stmt_handler_; }
  DeclHandler& decl_handler() { return decl_handler_; }

 private:
  TypeHandler type_handler_;
  ExprHandler expr_handler_;
  StmtHandler stmt_handler_;
  DeclHandler decl_handler_;
};

}  // namespace type_checker
}  // namespace lang

#endif /* lang_type_checker_type_resolver_h */

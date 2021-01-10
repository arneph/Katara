//
//  variable_handler.h
//  Katara
//
//  Created by Arne Philipeit on 10/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_variable_handler_h
#define lang_type_checker_variable_handler_h

#include <memory>

#include "lang/processors/issues/issues.h"
#include "lang/processors/type_checker/base_handler.h"
#include "lang/representation/ast/ast.h"
#include "lang/representation/types/info.h"
#include "lang/representation/types/info_builder.h"
#include "lang/representation/types/objects.h"
#include "lang/representation/types/types.h"

namespace lang {
namespace type_checker {

class VariableHandler final : public BaseHandler {
 public:
  bool ProcessVariable(types::Variable* variable, types::Type* type, ast::Expr* value);
  bool ProcessVariables(std::vector<types::Variable*> variables, types::Type* type,
                        ast::Expr* value);

 private:
  VariableHandler(class TypeResolver& type_resolver, types::InfoBuilder& info_builder,
                  std::vector<issues::Issue>& issues)
      : BaseHandler(type_resolver, info_builder, issues) {}

  bool ProcessVariableDefinitions(std::vector<types::Variable*> variables, types::Type* type,
                                  ast::Expr* value);

  friend class TypeResolver;
};

}  // namespace type_checker
}  // namespace lang

#endif /* lang_type_checker_variable_handler_h */

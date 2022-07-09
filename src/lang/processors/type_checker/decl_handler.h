//
//  decl_handler.h
//  Katara
//
//  Created by Arne Philipeit on 3/28/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_decl_handler_h
#define lang_type_checker_decl_handler_h

#include "src/lang/processors/issues/issues.h"
#include "src/lang/processors/type_checker/base_handler.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/types/info.h"
#include "src/lang/representation/types/info_builder.h"
#include "src/lang/representation/types/objects.h"
#include "src/lang/representation/types/types.h"

namespace lang {
namespace type_checker {

class DeclHandler final : public BaseHandler {
 public:
  bool ProcessTypeName(types::TypeName* type_name, ast::TypeSpec* type_spec);
  bool ProcessTypeParametersOfTypeName(types::TypeName* type_name, ast::TypeSpec* type_spec);
  bool ProcessUnderlyingTypeOfTypeName(types::TypeName* type_name, ast::TypeSpec* type_spec);

  bool ProcessConstant(types::Constant* constant, ast::Expr* type, ast::Expr* value, int64_t iota);
  bool ProcessVariable(types::Variable* variable, ast::Expr* type, ast::Expr* value);
  bool ProcessVariables(std::vector<types::Variable*> variables, ast::Expr* type, ast::Expr* value);

  bool ProcessFunction(types::Func* func, ast::FuncDecl* func_decl);

 private:
  DeclHandler(class TypeResolver& type_resolver, types::InfoBuilder& info_builder,
              issues::IssueTracker& issues)
      : BaseHandler(type_resolver, info_builder, issues) {}

  types::Variable* EvaluateExprReceiver(ast::ExprReceiver* expr_receiver,
                                        types::Func* instance_method);
  types::Type* EvaluateTypeReceiver(ast::TypeReceiver* type_receiver, types::Func* type_method);
  types::Type* EvalutateReceiverTypeInstance(ast::Ident* type_name,
                                             std::vector<ast::Ident*> type_param_names,
                                             types::Func* method);

  friend class TypeResolver;
};

}  // namespace type_checker
}  // namespace lang

#endif /* lang_type_checker_decl_handler_hpp */

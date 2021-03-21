//
//  type_handler.h
//  Katara
//
//  Created by Arne Philipeit on 11/8/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_type_checker_type_handler_h
#define lang_type_checker_type_handler_h

#include <vector>

#include "lang/processors/issues/issues.h"
#include "lang/processors/type_checker/base_handler.h"
#include "lang/representation/ast/ast.h"
#include "lang/representation/types/info.h"
#include "lang/representation/types/info_builder.h"
#include "lang/representation/types/objects.h"
#include "lang/representation/types/types.h"

namespace lang {
namespace type_checker {

class TypeHandler final : public BaseHandler {
 public:
  bool ProcessTypeName(types::TypeName* type_name, ast::TypeSpec* type_spec);
  bool ProcessTypeParametersOfTypeName(types::TypeName* type_name, ast::TypeSpec* type_spec);
  bool ProcessUnderlyingTypeOfTypeName(types::TypeName* type_name, ast::TypeSpec* type_spec);
  bool ProcessFuncDecl(types::Func* func, ast::FuncDecl* func_decl);

  bool ProcessTypeArgs(std::vector<ast::Expr*> type_args);
  bool ProcessTypeExpr(ast::Expr* type_expr);

 private:
  TypeHandler(class TypeResolver& type_resolver, types::InfoBuilder& info_builder,
              issues::IssueTracker& issues)
      : BaseHandler(type_resolver, info_builder, issues) {}

  bool ProcessTypeParameters(types::TypeName* type_name, ast::TypeSpec* type_spec);
  bool ProcessUnderlyingType(types::TypeName* type_name, ast::TypeSpec* type_spec);

  bool ProcessFuncDefinition(types::Func* func, ast::FuncDecl* func_decl);

  bool EvaluateTypeExpr(ast::Expr* expr);
  bool EvaluateTypeIdent(ast::Ident* ident);
  bool EvaluatePointerType(ast::UnaryExpr* pointer_type);
  bool EvaluateArrayType(ast::ArrayType* array_type);
  bool EvaluateFuncType(ast::FuncType* func_type);
  bool EvaluateInterfaceType(ast::InterfaceType* interface_type);
  bool EvaluateStructType(ast::StructType* struct_type);
  bool EvaluateTypeInstance(ast::TypeInstance* type_instance);

  std::vector<types::TypeParameter*> EvaluateTypeParameters(ast::TypeParamList* type_parameters);
  types::TypeParameter* EvaluateTypeParameter(ast::TypeParam* type_parameter);

  types::Func* EvaluateMethodSpec(ast::MethodSpec* method_spec, types::Interface* interface);
  types::Tuple* EvaluateTuple(ast::FieldList* field_list);
  std::vector<types::Variable*> EvaluateFieldList(ast::FieldList* field_list);
  std::vector<types::Variable*> EvaluateField(ast::Field* field);

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

#endif /* lang_type_checker_type_handler_h */

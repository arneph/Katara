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
  // Evaluatest the given type expressions. If successful, the types are returned, otherwise an
  // empty vector.
  std::vector<types::Type*> EvaluateTypeExprs(const std::vector<ast::Expr*>& exprs);
  // Evaluates the given type expression. if successful, the type is returned, otherise nullptr.
  types::Type* EvaluateTypeExpr(ast::Expr* expr);

  // Evaluates the given field list. If successful, the defined tuple is returned, otherwise
  // nullptr.
  types::Tuple* EvaluateTuple(ast::FieldList* field_list);

  // Evaluates the given type parameter list and returns the type parameters if successful,
  // otherwise an empty vector.
  std::vector<types::TypeParameter*> EvaluateTypeParameters(ast::TypeParamList* type_parameters);

 private:
  TypeHandler(class TypeResolver& type_resolver, types::InfoBuilder& info_builder,
              issues::IssueTracker& issues)
      : BaseHandler(type_resolver, info_builder, issues) {}

  bool ProcessTypeParameters(types::TypeName* type_name, ast::TypeSpec* type_spec);
  bool ProcessUnderlyingType(types::TypeName* type_name, ast::TypeSpec* type_spec);

  bool ProcessFuncDefinition(types::Func* func, ast::FuncDecl* func_decl);

  types::Type* EvaluateTypeIdent(ast::Ident* ident);
  types::Pointer* EvaluatePointerType(ast::UnaryExpr* pointer_type);
  types::Container* EvaluateArrayType(ast::ArrayType* array_type);
  types::Signature* EvaluateFuncType(ast::FuncType* func_type);
  types::Interface* EvaluateInterfaceType(ast::InterfaceType* interface_type);
  types::Struct* EvaluateStructType(ast::StructType* struct_type);
  types::TypeInstance* EvaluateTypeInstance(ast::TypeInstance* type_instance);

  types::TypeParameter* EvaluateTypeParameter(ast::TypeParam* type_parameter);

  types::Func* EvaluateMethodSpec(ast::MethodSpec* method_spec, types::Interface* interface);
  std::vector<types::Variable*> EvaluateFieldList(ast::FieldList* field_list);
  std::vector<types::Variable*> EvaluateField(ast::Field* field);

  friend class TypeResolver;
};

}  // namespace type_checker
}  // namespace lang

#endif /* lang_type_checker_type_handler_h */

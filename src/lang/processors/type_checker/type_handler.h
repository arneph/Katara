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

#include "lang/representation/ast/ast.h"
#include "lang/representation/types/types.h"
#include "lang/representation/types/objects.h"
#include "lang/representation/types/info.h"
#include "lang/representation/types/info_builder.h"
#include "lang/processors/issues/issues.h"

namespace lang {
namespace type_checker {

class TypeHandler {
public:
    static bool ProcessTypeName(types::TypeName *type_name,
                                ast::TypeSpec *type_spec,
                                types::InfoBuilder& info_builder,
                                std::vector<issues::Issue>& issues);
    static bool ProcessTypeParametersOfTypeName(types::TypeName *type_name,
                                                ast::TypeSpec *type_spec,
                                                types::InfoBuilder& info_builder,
                                                std::vector<issues::Issue>& issues);
    static bool ProcessUnderlyingTypeOfTypeName(types::TypeName *type_name,
                                                ast::TypeSpec *type_spec,
                                                types::InfoBuilder& info_builder,
                                                std::vector<issues::Issue>& issues);
    static bool ProcessFuncDecl(types::Func *func,
                                ast::FuncDecl *func_decl,
                                types::InfoBuilder& info_builder,
                                std::vector<issues::Issue>& issues);
    
    static bool ProcessTypeArgs(std::vector<ast::Expr *> type_args,
                                types::InfoBuilder& info_builder,
                                std::vector<issues::Issue>& issues);
    static bool ProcessTypeExpr(ast::Expr *type_expr,
                                types::InfoBuilder& info_builder,
                                std::vector<issues::Issue>& issues);
    
private:
    TypeHandler(types::InfoBuilder& info_builder,
                std::vector<issues::Issue>& issues)
    : info_(info_builder.info()), info_builder_(info_builder), issues_(issues) {}
    
    bool ProcessTypeParameters(types::TypeName *type_name,
                               ast::TypeSpec *type_spec);
    bool ProcessUnderlyingType(types::TypeName *type_name,
                               ast::TypeSpec *type_spec);
    
    bool ProcessFuncDefinition(types::Func *func,
                               ast::FuncDecl *func_decl);
    
    bool EvaluateTypeExpr(ast::Expr *expr);
    bool EvaluateTypeIdent(ast::Ident *ident);
    bool EvaluatePointerType(ast::UnaryExpr *pointer_type);
    bool EvaluateArrayType(ast::ArrayType *array_type);
    bool EvaluateFuncType(ast::FuncType *func_type);
    bool EvaluateInterfaceType(ast::InterfaceType *interface_type);
    bool EvaluateStructType(ast::StructType *struct_type);
    bool EvaluateTypeInstance(ast::TypeInstance *type_instance);
        
    std::vector<types::TypeParameter *> EvaluateTypeParameters(ast::TypeParamList *type_parameters);
    types::TypeParameter * EvaluateTypeParameter(ast::TypeParam *type_parameter);
    
    types::Func * EvaluateMethodSpec(ast::MethodSpec *method_spec,
                                     types::Interface *interface);
    types::Tuple * EvaluateTuple(ast::FieldList *field_list);
    std::vector<types::Variable *> EvaluateFieldList(ast::FieldList *field_list);
    std::vector<types::Variable *> EvaluateField(ast::Field *field);

    types::Variable * EvaluateExprReceiver(ast::ExprReceiver *expr_receiver,
                                           types::Func *instance_method);
    types::Type * EvaluateTypeReceiver(ast::TypeReceiver *type_receiver,
                                       types::Func *type_method);

    types::Type * EvalutateReceiverTypeInstance(ast::Ident *type_name,
                                                std::vector<ast::Ident *> type_param_names,
                                                types::Func *method);
    
    types::Info *info_;
    types::InfoBuilder&info_builder_;
    std::vector<issues::Issue>& issues_;
};

}
}

#endif /* lang_type_checker_type_handler_h */

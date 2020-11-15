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
#include "lang/processors/issues/issues.h"

namespace lang {
namespace type_checker {

class TypeHandler {
public:
    static bool ProcessTypeName(types::TypeName *type_name,
                                ast::TypeSpec *type_spec,
                                types::TypeInfo *info,
                                std::vector<issues::Issue>& issues);
    
    static bool ProcessFuncDecl(types::Func *func,
                                ast::FuncDecl *func_decl,
                                types::TypeInfo *info,
                                std::vector<issues::Issue>& issues);
    
    static bool ProcessTypeExpr(ast::Expr *type_expr,
                                types::TypeInfo *info,
                                std::vector<issues::Issue>& issues);
    
private:
    TypeHandler(types::TypeInfo *info,
                std::vector<issues::Issue>& issues)
    : info_(info), issues_(issues) {}
    
    bool ProcessTypeDefinition(types::TypeName *type_name,
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
    
    
    types::TypeTuple * EvaluateTypeParameters(ast::TypeParamList *type_parameters);
    types::NamedType * EvaluateTypeParameter(ast::TypeParam *type_parameter);
    
    types::Func * EvaluateMethodSpec(ast::MethodSpec *method_spec);
    types::Tuple * EvaluateTuple(ast::FieldList *field_list);
    std::vector<types::Variable *> EvaluateFieldList(ast::FieldList *field_list);
    std::vector<types::Variable *> EvaluateField(ast::Field *field);

    types::Variable * EvaluateReceiver(ast::FieldList *receiver_expr);

    types::TypeInfo *info_;
    std::vector<issues::Issue>& issues_;
};

}
}

#endif /* lang_type_checker_type_handler_h */

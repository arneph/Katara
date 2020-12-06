//
//  info_builder.h
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_info_builder_h
#define lang_types_info_builder_h

#include <string>
#include <unordered_map>
#include <vector>

#include "lang/representation/positions/positions.h"
#include "lang/representation/constants/constants.h"
#include "lang/representation/ast/ast.h"
#include "lang/representation/types/types.h"
#include "lang/representation/types/objects.h"
#include "lang/representation/types/scope.h"
#include "lang/representation/types/package.h"
#include "lang/representation/types/selection.h"
#include "lang/representation/types/initializer.h"
#include "lang/representation/types/info.h"

namespace lang {
namespace types {

class InfoBuilder {
public:
    typedef std::unordered_map<TypeParameter *, Type *> TypeParamsToArgsMap;
    
    Info * info() const;
    
    void CreateUniverse();
    
    Pointer * CreatePointer(Pointer::Kind kind, Type *element_type);
    Array * CreateArray(Type *element_type, uint64_t length);
    Slice * CreateSlice(Type *element_type);
    TypeInstance * CreateTypeInstance(NamedType *instantiated_type,
                                      std::vector<Type *> type_args);
    Tuple * CreateTuple(std::vector<Variable *> variables);
    Signature * CreateSignature(Variable *receiver,
                                std::vector<TypeParameter *> type_parameters,
                                Tuple *parameters,
                                Tuple *results);
    Struct * CreateStruct(std::vector<Variable *> fields);
    Interface * CreateInterface(std::vector<NamedType *> embedded_interfaces,
                                std::vector<Func *> methods);
    
    TypeInstance * InstantiateNamedType(NamedType *parameterized_type,
                                        TypeParamsToArgsMap& type_params_to_args);
    Signature * InstantiateSignature(Signature * parameterized_signature,
                                     TypeParamsToArgsMap& type_params_to_args);
    
    void SetInterfaceOfTypeParameter(TypeParameter *type_parameter,
                                     Type *interface);
    void SetTypeParametersOfNamedType(NamedType *named_type,
                                      std::vector<TypeParameter *> type_parameters);
    void SetUnderlyingTypeOfNamedType(NamedType *named_type,
                                      Type *underlying_type);
    
    TypeName * CreateTypeNameForTypeParameter(Scope *parent,
                                              Package *package,
                                              pos::pos_t position,
                                              std::string name);
    TypeName * CreateTypeNameForNamedType(Scope *parent,
                                          Package *package,
                                          pos::pos_t position,
                                          std::string name,
                                          bool is_alias);
    Constant * CreateConstant(Scope *parent,
                              Package *package,
                              pos::pos_t position,
                              std::string name);
    Variable * CreateVariable(Scope *parent,
                              Package *package,
                              pos::pos_t position,
                              std::string name,
                              bool is_embedded,
                              bool is_field);
    Func * CreateFunc(Scope *parent,
                      Package *package,
                      pos::pos_t position,
                      std::string name);
    Label * CreateLabel(Scope *parent,
                        Package *package,
                        pos::pos_t position,
                        std::string name);
    PackageName * CreatePackageName(Scope *parent,
                                    Package *package,
                                    pos::pos_t position,
                                    std::string name,
                                    Package *referenced_package);
    
    void SetObjectType(Object *object, Type *type);
    void SetConstantValue(Constant *constant, constants::Value value);
    
    void SetExprType(ast::Expr *expr, Type *type);
    void SetExprKind(ast::Expr *expr, ExprKind kind);
    void SetExprConstantValue(ast::Expr *expr, constants::Value value);
    
    void SetDefinedObject(ast::Ident *ident, Object *object);
    void SetUsedObject(ast::Ident *ident, Object *object);
    void SetImplicitObject(ast::Node *node, Object *object);
    
    Scope * CreateScope(ast::Node *node, Scope *parent);
    void AddObjectToScope(Scope *scope, Object *object);
    
    Package * CreatePackage(std::string path, std::string name);
    void AddImportToPackage(Package *importer, Package *imported);
    
    void AddInitializer(std::vector<Variable *> lhs, ast::Expr *rhs);
    
    Selection * CreateSelection(Selection::Kind kind,
                                Type *receiver_type,
                                Type *type,
                                Object *object);
    
private:
    InfoBuilder(Info *info);
    
    void CreatePredeclaredTypes();
    void CreatePredeclaredConstants();
    void CreatePredeclaredNil();
    void CreatePredeclaredFuncs();
    
    TypeParameter * CreateTypeParameter(std::string name);
    NamedType * CreateNamedType(bool is_alias,
                                std::string name);
    
    Type * InstantiateType(Type *parameterized_type,
                           TypeParamsToArgsMap& type_params_to_args);
    std::vector<Type *> InstantiateTypeParameters(std::vector<TypeParameter *> type_params,
                                                  TypeParamsToArgsMap& type_params_to_args);
    
    void CheckObjectArgs(Scope *parent,
                         Package *package) const;
    
    Info *info_;
    
    friend Info;
};

}
}

#endif /* lang_types_info_builder_h */

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
#include "lang/representation/types/initializer.h"
#include "lang/representation/types/info.h"

namespace lang {
namespace types {

class InfoBuilder {
public:
    Info * info() const;
    
    void CreateUniverse();
    
    Pointer * CreatePointer(Pointer::Kind kind, Type *element_type);
    Array * CreateArray(Type *element_type, uint64_t length);
    Slice * CreateSlice(Type *element_type);
    TypeTuple * CreateTypeTuple(std::vector<NamedType *> types);
    NamedType * CreateNamedType(bool is_type_parameter,
                                std::string name,
                                Type *type,
                                TypeTuple *type_parameters);
    TypeInstance * CreateTypeInstance(Type *instantiated_type,
                                      std::vector<Type *> type_args);
    Tuple * CreateTuple(std::vector<Variable *> variables);
    Signature * CreateSignature(Variable *receiver,
                                TypeTuple *type_parameters,
                                Tuple *parameters,
                                Tuple *results);
    Struct * CreateStruct(std::vector<Variable *> fields);
    Interface * CreateInterface(std::vector<NamedType *> embedded_interfaces,
                                std::vector<Func *> methods);
    Type * InstantiateType(Type *parameterized_type,
                           std::unordered_map<NamedType *, Type *> type_params_to_args);
    
    TypeName * CreateTypeName(Scope *parent,
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
    
private:
    InfoBuilder(Info *info);
    
    void CreatePredeclaredTypes();
    void CreatePredeclaredConstants();
    void CreatePredeclaredNil();
    void CreatePredeclaredFuncs();
    
    void CheckObjectArgs(Scope *parent,
                         Package *package) const;
    
    Info *info_;
    
    friend Info;
};

}
}

#endif /* lang_types_info_builder_h */

//
//  types.h
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_h
#define lang_types_h

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "lang/representation/positions/positions.h"
#include "lang/representation/ast/ast.h"
#include "lang/representation/constants/constants.h"

namespace lang {
namespace type_checker {
class UniverseBuilder;
class IdentifierResolver;
class InitHandler;
class ConstantHandler;

}

namespace types {

class Type;
class Object;
class Variable;
class Func;
class Scope;

class Type {
public:
    virtual ~Type() {}
    
    virtual Type * Underlying() = 0;
    
    virtual std::string ToString() const = 0;
};

class Basic : public Type {
public:
    typedef enum : int8_t {
        kBool,
        kInt,
        kInt8,
        kInt16,
        kInt32,
        kInt64,
        kUint,
        kUint8,
        kUint16,
        kUint32,
        kUint64,
        kString,
        
        kUntypedBool,
        kUntypedInt,
        kUntypedRune,
        kUntypedString,
        kUntypedNil,
        
        kByte = kUint8,
        kRune = kInt32,
    } Kind;
    typedef enum : int8_t {
        kIsBoolean = 1 << 0,
        kIsInteger = 1 << 1,
        kIsUnsigned = 1 << 2,
        kIsString = 1 << 3,
        kIsUntyped = 1 << 4,
        
        kIsOrdered = kIsInteger | kIsString,
        kIsNumeric = kIsInteger,
        kIsConstant = kIsBoolean | kIsNumeric | kIsString,
    } Info;
    
    ~Basic() {}
    
    Kind kind() const;
    Info info() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    Basic(Kind kind, Info info);
    
    Kind kind_;
    Info info_;
    
    friend type_checker::UniverseBuilder;
};

class Pointer : public Type {
public:
    enum class Kind : int8_t {
        kStrong,
        kWeak,
    };
    
    ~Pointer() {}
    
    Kind kind() const;
    Type * element_type() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    Pointer() {}
    
    Kind kind_;
    Type *element_type_;
};

class Array : public Type {
public:
    ~Array() {}
    
    Type * element_type() const;
    uint64_t length() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    Array() {}
    
    Type *element_type_;
    uint64_t length_;
};

class Slice : public Type {
public:
    ~Slice() {}
    
    Type * element_type() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    Slice() {}
    
    Type *element_type_;
};

class NamedType;

class TypeTuple : public Type {
public:
    ~TypeTuple() {}
    
    const std::vector<NamedType *>& types() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    TypeTuple() {}
    
    std::vector<NamedType *> types_;
};

class NamedType : public Type {
public:
    ~NamedType() {}
    
    bool is_type_parameter() const;
    std::string name() const;
    Type * type() const;
    TypeTuple * type_parameters() const;
    
    Type * Underlying();
    
    std::string ToString() const;
private:
    NamedType() {}
    
    bool is_type_parameter_;
    std::string name_;
    Type *type_;
    TypeTuple *type_parameters_;
};

class TypeInstance : public Type {
public:
    ~TypeInstance() {}
    
    Type * instantiated_type() const;
    const std::vector<Type *>& type_args() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    TypeInstance() {}
    
    Type *instantiated_type_;
    std::vector<Type *> type_args_;
};

class Tuple : public Type {
public:
    ~Tuple() {}
    
    const std::vector<Variable *>& variables() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    Tuple() {}
    
    std::vector<Variable *> variables_;
};

class Signature : public Type {
public:
    ~Signature() {}
    
    Variable * receiver() const;
    TypeTuple * type_parameters() const;
    Tuple * parameters() const;
    Tuple * results() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    Signature() {}
    
    Variable *receiver_;
    TypeTuple *type_parameters_;
    Tuple *parameters_;
    Tuple *results_;
};

class Struct : public Type {
public:
    ~Struct() {}
    
    const std::vector<Variable *>& fields() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    Struct() {}
    
    std::vector<Variable *> fields_;
};

class Interface : public Type {
public:
    ~Interface() {}
    
    const std::vector<NamedType *>& embedded_interfaces() const;
    const std::vector<Func *>& methods() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    Interface() {}
    
    std::vector<NamedType *> embedded_interfaces_;
    std::vector<Func *> methods_;
};

class Package;

class Object {
public:
    virtual ~Object() {};
    
    Scope * parent() const;
    Package * package() const;
    pos::pos_t position() const;
    
    std::string name() const;
    Type * type() const;
    
    virtual std::string ToString() const = 0;
    
protected:
    Object() {}
    
    Scope *parent_;
    Package *package_;
    pos::pos_t position_;
    std::string name_;
    Type *type_;
    
    friend type_checker::IdentifierResolver;
    friend type_checker::InitHandler;
    friend type_checker::ConstantHandler;
};

class TypeName : public Object {
public:
    ~TypeName() {}
    
    std::string ToString() const;
    
private:
    TypeName() {}
    
    friend type_checker::UniverseBuilder;
    friend type_checker::IdentifierResolver;
};

class Constant : public Object {
public:
    ~Constant() {}
    
    constants::Value value() const;
    
    std::string ToString() const;
    
private:
    Constant();
    
    constants::Value value_;
    
    friend type_checker::UniverseBuilder;
    friend type_checker::IdentifierResolver;
    friend type_checker::ConstantHandler;
};

class Variable : public Object {
public:
    ~Variable() {}
    
    bool is_embedded() const;
    bool is_field() const;
    
    std::string ToString() const;
    
private:
    Variable() {}
    
    bool is_embedded_;
    bool is_field_;
    
    friend type_checker::IdentifierResolver;
};

class Func : public Object {
public:
    ~Func() {}
    
    std::string ToString() const;
    
private:
    Func() {}
    
    friend type_checker::IdentifierResolver;
};

class Nil : public Object {
public:
    ~Nil() {}
    
    std::string ToString() const;
    
private:
    Nil() {}
    
    friend type_checker::UniverseBuilder;
};

class Label : public Object {
public:
    ~Label() {}
    
    std::string ToString() const;
    
private:
    Label() {}
    
    friend type_checker::IdentifierResolver;
};

class Builtin : public Object {
public:
    ~Builtin() {}
    
    std::string ToString() const;
    
private:
    typedef enum : uint8_t {
        kLen,
        kMake,
        kNew,
    } Kind;
    
    Builtin(Kind kind);
    
    Kind kind_;
};

class PackageName : public Object {
public:
    ~PackageName() {}
    
    Package * referenced_package() const;
    
    std::string ToString() const;
    
private:
    Package *referenced_package_;
    
    friend type_checker::IdentifierResolver;
};

class Scope {
public:
    ~Scope() {}
    
    Scope * parent() const;
    const std::vector<Scope *>& children() const;
    const std::unordered_map<std::string, Object *>& named_objects() const;
    const std::unordered_set<Object *>& unnamed_objects() const;
    
    Object * Lookup(std::string name) const;
    Object * Lookup(std::string name, const Scope*& defining_scope) const;
    
private:
    Scope() {}
    
    Scope *parent_;
    std::vector<Scope *> children_;
    std::unordered_map<std::string, Object *> named_objects_;
    std::unordered_set<Object *> unnamed_objects_;
    
    friend type_checker::UniverseBuilder;
    friend type_checker::IdentifierResolver;
};

class Initializer {
public:
    ~Initializer() {}
    
    const std::vector<Variable *>& lhs() const;
    ast::Expr * rhs() const;
    
private:
    Initializer() {}
    
    std::vector<Variable *> lhs_;
    ast::Expr *rhs_;
    
    friend type_checker::InitHandler;
};

class Package {
public:
    ~Package() {}
    
    std::string path() const;
    std::string name() const;
    Scope *scope() const;
    const std::unordered_set<Package *>& imports() const;
    
private:
    Package() {}
    
    std::string path_;
    std::string name_;
    Scope *scope_;
    std::unordered_set<Package *> imports_;
    
    friend type_checker::IdentifierResolver;
};

class TypeInfo {
public:
    const std::unordered_map<ast::Expr *, Type *>& types() const;
    const std::unordered_map<ast::Expr *, constants::Value>& constant_values() const;
    const std::unordered_map<ast::Ident *, Object *>& definitions() const;
    const std::unordered_map<ast::Ident *, Object *>& uses() const;
    const std::unordered_map<ast::Node *, Object *>& implicits() const;
    const std::unordered_map<ast::Node *, Scope *>& scopes() const;
    const std::unordered_set<Package *>& packages() const;
    
    Scope * universe() const;
    
    Object * ObjectOf(ast::Ident *ident) const;
    Object * DefinitionOf(ast::Ident *ident) const;
    Object * UseOf(ast::Ident *ident) const;
    Object * ImplicitOf(ast::Node *node) const;
    
    Scope * ScopeOf(ast::Node *node) const;
    
    Type * TypeOf(ast::Expr *expr) const;
    
private:
    std::vector<std::unique_ptr<Type>> type_unique_ptrs_;
    std::vector<std::unique_ptr<Object>> object_unique_ptrs_;
    std::vector<std::unique_ptr<Scope>> scope_unique_ptrs_;
    std::vector<std::unique_ptr<Initializer>> initializer_unique_ptrs_;
    std::vector<std::unique_ptr<Package>> package_unique_ptrs_;
    
    std::unordered_map<ast::Expr *, Type *> types_;
    std::unordered_map<ast::Expr *, constants::Value> constant_values_;
    
    std::unordered_map<ast::Ident *, Object *> definitions_;
    std::unordered_map<ast::Ident *, Object *> uses_;
    std::unordered_map<ast::Node *, Object *> implicits_;
    
    std::unordered_map<ast::Node *, Scope *> scopes_;
    
    std::vector<Initializer *> init_order_;
    
    std::unordered_set<Package *> packages_;
    
    Scope *universe_;
    std::unordered_map<types::Basic::Kind, types::Basic*> basic_types_;
    
    friend type_checker::UniverseBuilder;
    friend type_checker::IdentifierResolver;
    friend type_checker::InitHandler;
    friend type_checker::ConstantHandler;
};

}
}

#endif /* lang_types_h */

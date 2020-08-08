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

#include "lang/positions.h"
#include "lang/ast.h"
#include "lang/constant.h"

namespace lang {
namespace type_checker {
class TypeChecker;

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
        
        kUntypedBool,
        kUntypedInt,
        kUntypedNil,
        
        kByte = kUint8,
    } Kind;
    
    ~Basic() {}
    
    Kind kind() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    Basic(Kind kind);
    
    Kind kind_;
    
    friend type_checker::TypeChecker;
};

class Pointer : public Type {
public:
    ~Pointer() {}
    
    Type * element_type() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    Pointer() {}
    
    Type *element_type_;
    
    friend type_checker::TypeChecker;
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
    
    friend type_checker::TypeChecker;
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

    friend type_checker::TypeChecker;
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
    
    friend type_checker::TypeChecker;
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
    
    friend type_checker::TypeChecker;
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
    
    friend type_checker::TypeChecker;
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
    
    friend type_checker::TypeChecker;
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
    
    friend type_checker::TypeChecker;
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
    
    friend type_checker::TypeChecker;
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
    
    friend type_checker::TypeChecker;
};

class Object {
public:
    virtual ~Object() {};
    
    Scope * parent() const;
    pos::pos_t position() const;
    std::string name() const;
    Type * type() const;
    
    virtual std::string ToString() const = 0;
    
protected:
    Object() {}
    
    Scope *parent_;
    pos::pos_t position_;
    std::string name_;
    Type *type_;
    
    friend type_checker::TypeChecker;
};

class TypeName : public Object {
public:
    ~TypeName() {}
    
    std::string ToString() const;
    
private:
    TypeName() {}
    
    friend type_checker::TypeChecker;
};

class Constant : public Object {
public:
    ~Constant() {}
    
    constant::Value value() const;
    
    std::string ToString() const;
    
private:
    Constant();
    
    constant::Value value_;
    
    friend type_checker::TypeChecker;
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
    
    friend type_checker::TypeChecker;
};

class Func : public Object {
public:
    ~Func() {}
    
    std::string ToString() const;
    
private:
    Func() {}
    
    friend type_checker::TypeChecker;
};

class Nil : public Object {
public:
    ~Nil() {}
    
    std::string ToString() const;
    
private:
    Nil() {}
    
    friend type_checker::TypeChecker;
};

class Label : public Object {
public:
    ~Label() {}
    
    std::string ToString() const;
    
private:
    Label() {}
    
    friend type_checker::TypeChecker;
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
    
    friend type_checker::TypeChecker;
};

class Scope {
public:
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
    
    friend type_checker::TypeChecker;
};

class TypeInfo {
public:
    const std::unordered_map<ast::Expr *, Type *>& types() const;
    const std::unordered_map<ast::Expr *, constant::Value *>& constant_values() const;
    const std::unordered_map<ast::Ident *, Object *>& definitions() const;
    const std::unordered_map<ast::Ident *, Object *>& uses() const;
    const std::unordered_map<ast::Node *, Object *>& implicits() const;
    const std::unordered_map<ast::Node *, Scope *>& scopes() const;
    
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
    
    std::unordered_map<ast::Expr *, Type *> types_;
    std::unordered_map<ast::Expr *, constant::Value *> constant_values_;
    
    std::unordered_map<ast::Ident *, Object *> definitions_;
    std::unordered_map<ast::Ident *, Object *> uses_;
    std::unordered_map<ast::Node *, Object *> implicits_;
    
    std::unordered_map<ast::Node *, Scope *> scopes_;
    
    Scope *universe_;
    
    friend type_checker::TypeChecker;
};

}
}

#endif /* lang_types_h */

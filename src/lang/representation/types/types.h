//
//  types.h
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_types_h
#define lang_types_types_h

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace lang {
namespace types {

class Variable;
class Func;

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
    Basic();
    
    Kind kind_;
    Info info_;
    
    friend class InfoBuilder;
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
    
    friend class InfoBuilder;
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
    
    friend class InfoBuilder;
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
    
    friend class InfoBuilder;
};

class TypeParameter : public Type {
public:
    ~TypeParameter() {}
    
    std::string name() const;
    Type * interface() const;
    // Note: interface might return a NamedType, but the Underlying type should always be Interface
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    TypeParameter() {}
    
    std::string name_;
    Type *interface_;
    
    friend class InfoBuilder;
};

class NamedType : public Type {
public:
    ~NamedType() {}
    
    bool is_alias() const;
    std::string name() const;
    Type * type() const;
    const std::vector<TypeParameter *>& type_parameters() const;
    const std::unordered_map<std::string, Func *>& methods() const;
    
    Type * Underlying();
    
    std::string ToString() const;
private:
    NamedType() {}
    
    bool is_alias_;
    std::string name_;
    Type *type_;
    std::vector<TypeParameter *> type_parameters_;
    std::unordered_map<std::string, Func *> methods_;
    
    friend class InfoBuilder;
};

class TypeInstance : public Type {
public:
    ~TypeInstance() {}
    
    NamedType * instantiated_type() const;
    const std::vector<Type *>& type_args() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    TypeInstance() {}
    
    NamedType *instantiated_type_;
    std::vector<Type *> type_args_;
    
    friend class InfoBuilder;
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
    
    friend class InfoBuilder;
};

class Signature : public Type {
public:
    ~Signature() {}
    
    Variable * receiver() const;
    const std::vector<TypeParameter *>& type_parameters() const;
    Tuple * parameters() const;
    Tuple * results() const;
    
    Type * Underlying();
    
    std::string ToString() const;
    
private:
    Signature() {}
    
    Variable *receiver_;
    std::vector<TypeParameter *> type_parameters_;
    Tuple *parameters_;
    Tuple *results_;
    
    friend class InfoBuilder;
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
    
    friend class InfoBuilder;
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
    
    friend class InfoBuilder;
};

}
}

#endif /* lang_types_types_h */

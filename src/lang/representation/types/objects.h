//
//  objects.h
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_objects_h
#define lang_types_objects_h

#include <string>

#include "lang/representation/positions/positions.h"
#include "lang/representation/constants/constants.h"
#include "lang/representation/ast/ast.h"
#include "lang/representation/types/types.h"

namespace lang {
namespace types {

class Scope;
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
    
    friend class InfoBuilder;
};

class TypeName : public Object {
public:
    ~TypeName() {}
    
    bool is_alias() const;
    
    std::string ToString() const;
    
private:
    TypeName() {}
    
    bool is_alias_;
    
    friend class InfoBuilder;
};

class Constant : public Object {
public:
    ~Constant() {}
    
    constants::Value value() const;
    
    std::string ToString() const;
    
private:
    Constant();
    
    constants::Value value_;
    
    friend class InfoBuilder;
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
    
    friend class InfoBuilder;
};

class Func : public Object {
public:
    ~Func() {}
    
    std::string ToString() const;
    
private:
    Func() {}
    
    friend class InfoBuilder;
};

class Nil : public Object {
public:
    ~Nil() {}
    
    std::string ToString() const;
    
private:
    Nil() {}
    
    friend class InfoBuilder;
};

class Label : public Object {
public:
    ~Label() {}
    
    std::string ToString() const;
    
private:
    Label() {}
    
    friend class InfoBuilder;
};

class Builtin : public Object {
public:
    enum class Kind {
        kLen,
        kMake,
        kNew,
    };
    
    ~Builtin() {}
    
    Kind kind() const;
    
    std::string ToString() const;
    
private:
    Builtin();
    
    Kind kind_;
    
    friend class InfoBuilder;
};

class PackageName : public Object {
public:
    ~PackageName() {}
    
    Package * referenced_package() const;
    
    std::string ToString() const;
    
private:
    Package *referenced_package_;
    
    friend class InfoBuilder;
};

}
}

#endif /* lang_types_objects_h */

//
//  objects.cc
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "objects.h"

#include "lang/representation/types/scope.h"
#include "lang/representation/types/package.h"

namespace lang {
namespace types {

Scope * Object::parent() const {
    return parent_;
}

Package * Object::package() const {
    return package_;
}

pos::pos_t Object::position() const {
    return position_;
}

std::string Object::name() const {
    return name_;
}

Type * Object::type() const {
    return type_;
}

bool TypeName::is_alias() const {
    return is_alias_;
}

std::string TypeName::ToString() const {
    if (is_alias_) {
        return "type " + name_ + " = " + type_->ToString();
    }
    return "type " + name_ + " " + type_->ToString();
}

Constant::Constant() : value_(false) {}

constants::Value Constant::value() const {
    return value_;
}

std::string Constant::ToString() const {
    return "const " + name_ + " " + type_->ToString() + " = " + value_.ToString();
}

bool Variable::is_embedded() const {
    return is_embedded_;
}

bool Variable::is_field() const {
    return is_field_;
}

std::string Variable::ToString() const {
    if (is_field_) {
        if (is_embedded_) {
            return type_->ToString();
        } else {
            return name_ + " " + type_->ToString();
        }
    }
    return "var " + name_ + " " + type_->ToString();
}

std::string Func::ToString() const {
    Signature * sig = dynamic_cast<Signature *>(type_);
    std::string s = "func ";
    if (sig->receiver()) {
        s += "(" + sig->receiver()->ToString() + ") ";
    }
    s += name_;
    if (sig->type_parameters()) {
        s += sig->type_parameters()->ToString();
    }
    s += "(";
    if (sig->parameters() != nullptr) {
        s += sig->parameters()->ToString();
    }
    s += ")";
    if (sig->results()) {
        s += " ";
        if (sig->results()->variables().size() == 1 &&
            sig->results()->variables().at(0)->name().empty()) {
            s += sig->results()->ToString();
        } else {
            s += "(" + sig->results()->ToString() + ")";
        }
    }
    return s;
}

std::string Nil::ToString() const {
    if (type_) {
        return "nil <" + type_->ToString() + ">";
    }
    return "nil";
}

std::string Label::ToString() const {
    return name_ + " (label)";
}

Builtin::Builtin() {}

Builtin::Kind Builtin::kind() const {
    return kind_;
}

std::string Builtin::ToString() const {
    switch (kind_) {
        case Kind::kLen:
            return "len()";
        case Kind::kMake:
            return "make<[]T>()";
        case Kind::kNew:
            return "new<T>()";
        default:
            break;
    }
}

Package * PackageName::referenced_package() const {
    return referenced_package_;
}

std::string PackageName::ToString() const {
    return name_;
}

}
}

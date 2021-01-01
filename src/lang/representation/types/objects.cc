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

bool Object::is_typed() const {
    ObjectKind kind = object_kind();
    return ObjectKind::kTypedObjectStart <= kind && kind <= ObjectKind::kTypedObjectEnd;
}

std::string Variable::ToString() const {
    std::string type_str = "<unknown type>";
    if (type() != nullptr) {
        type_str = type()->ToString(StringRep::kShort);
    }
    if (is_field_) {
        if (is_embedded_) {
            return type_str;
        } else {
            return name() + " " + type_str;
        }
    }
    return "var " + name() + " " + type_str;
}

std::string Func::ToString() const {
    Signature * sig = static_cast<Signature *>(type());
    std::string s = "func ";
    if (sig->expr_receiver() != nullptr) {
        s += "(" + sig->expr_receiver()->ToString() + ") ";
    } else if (sig->type_receiver() != nullptr) {
        s += "<" + sig->type_receiver()->ToString(StringRep::kShort) + "> ";
    }
    s += name();
    if (!sig->type_parameters().empty()) {
        s += "<";
        for (int i = 0; i < sig->type_parameters().size(); i++) {
            if (i > 0) {
                s += ", ";
            }
            s += sig->type_parameters().at(i)->ToString(StringRep::kShort);
        }
        s += ">";
    }
    s += "(";
    if (sig->parameters() != nullptr) {
        s += sig->parameters()->ToString(StringRep::kShort);
    }
    s += ")";
    if (sig->results()) {
        s += " ";
        if (sig->results()->variables().size() == 1 &&
            sig->results()->variables().at(0)->name().empty()) {
            s += sig->results()->ToString(StringRep::kShort);
        } else {
            s += "(" + sig->results()->ToString(StringRep::kShort) + ")";
        }
    }
    return s;
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

}
}

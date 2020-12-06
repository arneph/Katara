//
//  types.cc
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "types.h"

#include "lang/representation/types/objects.h"

namespace lang {
namespace types {

Basic::Basic() {}

Basic::Kind Basic::kind() const {
    return kind_;
}

Basic::Info Basic::info() const {
    return info_;
}

Type * Basic::Underlying() {
    return this;
}

std::string Basic::ToString() const {
    switch (kind_) {
        case kBool:
            return "bool";
        case kInt:
            return "int";
        case kInt8:
            return "int8";
        case kInt16:
            return "int16";
        case kInt32:
            return "int32";
        case kInt64:
            return "int64";
        case kUint:
            return "uint";
        case kUint8:
            return "uint8";
        case kUint16:
            return "uint16";
        case kUint32:
            return "uint32";
        case kUint64:
            return "uint64";
        case kString:
            return "string";
            
        case kUntypedBool:
            return "bool (untyped)";
        case kUntypedInt:
            return "int (untyped)";
        case kUntypedRune:
            return "rune (untyped)";
        case kUntypedString:
            return "string (untyped)";
        case kUntypedNil:
            return "nil (untyped)";
            
        default:
            throw "unexpected Basic::Kind";
    }
}

Pointer::Kind Pointer::kind() const {
    return kind_;
}

Type * Pointer::element_type() const {
    return element_type_;
}

Type * Pointer::Underlying() {
    return this;
}

std::string Pointer::ToString() const {
    switch (kind_) {
        case Kind::kStrong:
            return "*" + element_type_->ToString();
        case Kind::kWeak:
            return "%" + element_type_->ToString();
    }
}

Type * Array::element_type() const {
    return element_type_;
}

uint64_t Array::length() const {
    return length_;
}

Type * Array::Underlying() {
    return this;
}

std::string Array::ToString() const {
    return "[" + std::to_string(length()) + "]" + element_type_->ToString();
}

Type * Slice::element_type() const {
    return element_type_;
}

Type * Slice::Underlying() {
    return this;
}

std::string Slice::ToString() const {
    return "[]" + element_type_->ToString();
}

std::string TypeParameter::name() const {
    return name_;
}

Type * TypeParameter::interface() const {
    return interface_;
}

Type * TypeParameter::Underlying() {
    return interface_;
}

std::string TypeParameter::ToString() const {
    std::string s = name_;
    auto interface = dynamic_cast<Interface *>(interface_);
    if (interface == nullptr ||
        !interface->embedded_interfaces().empty() ||
        !interface->methods().empty()) {
        s += " " + interface_->ToString();
    }
    return s;
}

bool NamedType::is_alias() const {
    return is_alias_;
}

std::string NamedType::name() const {
    return name_;
}

Type * NamedType::type() const {
    return type_;
}

const std::vector<TypeParameter *>& NamedType::type_parameters() const {
    return type_parameters_;
}

const std::unordered_map<std::string, Func *>& NamedType::methods() const {
    return methods_;
}

Type * NamedType::Underlying() {
    return type_;
}

std::string NamedType::ToString() const {
    std::string s;
    if (is_alias_) {
        s += "=";
    }
    s += name_;
    if (!type_parameters_.empty()) {
        s += "<";
        for (size_t i = 0; i < type_parameters_.size(); i++) {
            if (i > 0) s += ", ";
            s += type_parameters_.at(i)->ToString();
        }
        s += ">";
    }
    return s;
}

NamedType * TypeInstance::instantiated_type() const {
    return instantiated_type_;
}

const std::vector<Type *>& TypeInstance::type_args() const {
    return type_args_;
}

Type * TypeInstance::Underlying() {
    return this;
}

std::string TypeInstance::ToString() const {
    std::string s = instantiated_type_->ToString() + "<";
    for (size_t i = 0; i < type_args_.size(); i++) {
        if (i > 0) s += ", ";
        s += type_args_.at(i)->ToString();
    }
    s += ">";
    return s;
}

const std::vector<Variable *>& Tuple::variables() const {
    return variables_;
}

Type * Tuple::Underlying() {
    return this;
}

std::string Tuple::ToString() const {
    std::string s;
    for (size_t i = 0; i < variables_.size(); i++) {
        if (i > 0) s += ", ";
        std::string name = variables_.at(i)->name();
        if (!name.empty()) {
            s += name + " ";
        }
        s += variables_.at(i)->type()->ToString();
    }
    return s;
}

Variable * Signature::receiver() const {
    return receiver_;
}

const std::vector<TypeParameter *>& Signature::type_parameters() const {
    return type_parameters_;
}

Tuple * Signature::parameters() const {
    return parameters_;
}

Tuple * Signature::results() const {
    return results_;
}

Type * Signature::Underlying() {
    return this;
}

std::string Signature::ToString() const {
    std::string s = "func ";
    if (receiver_) {
        s += "(" + receiver_->ToString() + ") ";
    }
    if (!type_parameters_.empty()) {
        s += "<";
        for (size_t i = 0; i < type_parameters_.size(); i++) {
            if (i > 0) s += ", ";
            s += type_parameters_.at(i)->ToString();
        }
        s += ">";
    }
    s += "(";
    if (parameters_ != nullptr) {
        s += parameters_->ToString();
    }
    s += ")";
    if (results_) {
        s += " ";
        if (results_->variables().size() == 1 &&
            results_->variables().at(0)->name().empty()) {
            s += results_->ToString();
        } else {
            s += "(" + results_->ToString() + ")";
        }
    }
    return s;
}

const std::vector<Variable *>& Struct::fields() const {
    return fields_;
}

Type * Struct::Underlying() {
    return this;
}

std::string Struct::ToString() const {
    std::string s = "struct{";
    for (size_t i = 0; i < fields_.size(); i++) {
        if (i > 0) s += "; ";
        std::string name = fields_.at(i)->name();
        if (!name.empty()) {
            s += name + " ";
        }
        s += fields_.at(i)->type()->ToString();
    }
    s += "}";
    return s;
}

const std::vector<NamedType *>& Interface::embedded_interfaces() const {
    return embedded_interfaces_;
}

const std::vector<Func *>& Interface::methods() const {
    return methods_;
}

Type * Interface::Underlying() {
    return this;
}

std::string Interface::ToString() const {
    std::string s = "interface {";
    for (size_t i = 0; i < embedded_interfaces_.size(); i++) {
        if (i > 0) s += "; ";
        s += embedded_interfaces_.at(i)->ToString();
    }
    for (size_t i = 0; i < methods_.size(); i++) {
        if (i > 0 || !embedded_interfaces_.empty()) s += "; ";
        s += methods_.at(i)->ToString();
    }
    s += "}";
    return s;
}

}
}

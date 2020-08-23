//
//  types.cc
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "types.h"

namespace lang {
namespace types {

Basic::Basic(Kind kind, Info info) : kind_(kind), info_(info) {}

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
        case kUntypedBool:
            return "bool (untyped)";
        case kUntypedInt:
            return "int (untyped)";
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

const std::vector<NamedType *>& TypeTuple::types() const {
    return types_;
}

Type * TypeTuple::Underlying() {
    return this;
}

std::string TypeTuple::ToString() const {
    std::string s = "<";
    for (size_t i = 0; i < types_.size(); i++) {
        if (i > 0) s += ", ";
        s += types_.at(i)->name() + " " + types_.at(i)->type()->ToString();
    }
    s += ">";
    return s;
}

bool NamedType::is_type_parameter() const {
    return is_type_parameter_;
}

std::string NamedType::name() const {
    return name_;
}

Type * NamedType::type() const {
    return type_;
}

TypeTuple * NamedType::type_parameters() const {
    return type_parameters_;
}

Type * NamedType::Underlying() {
    return type_;
}

std::string NamedType::ToString() const {
    if (type_parameters_) {
        return name_ + type_parameters_->ToString();
    }
    return name_;
}

Type * TypeInstance::instantiated_type() const {
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

TypeTuple * Signature::type_parameters() const {
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
    if (type_parameters_) {
        s += type_parameters_->ToString();
    }
    s += "(" + parameters_->ToString() + ")";
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

Scope * Object::parent() const {
    return parent_;
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

std::string TypeName::ToString() const {
    return "type " + name_ + " " + type_->ToString();
}

Constant::Constant() : value_(false) {}

constant::Value Constant::value() const {
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
    s += "(" + sig->parameters()->ToString() + ")";
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

Builtin::Builtin(Kind kind) : kind_(kind) {}

std::string Builtin::ToString() const {
    switch (kind_) {
        case kLen:
            return "len()";
        case kMake:
            return "make()";
        case kNew:
            return "new()";
        default:
            break;
    }
}

Scope * Scope::parent() const {
    return parent_;
}

const std::vector<Scope *>& Scope::children() const {
    return children_;
}

const std::unordered_map<std::string, Object *>& Scope::named_objects() const {
    return named_objects_;
}

const std::unordered_set<Object *>& Scope::unnamed_objects() const {
    return unnamed_objects_;
}

Object * Scope::Lookup(std::string name) const {
    auto it = named_objects_.find(name);
    if (it != named_objects_.end()) {
        return it->second;
    }
    if (parent_) {
        return parent_->Lookup(name);
    }
    return nullptr;
}

Object * Scope::Lookup(std::string name, const Scope*& scope) const {
    auto it = named_objects_.find(name);
    if (it != named_objects_.end()) {
        scope = this;
        return it->second;
    }
    if (parent_) {
        return parent_->Lookup(name, scope);
    }
    return nullptr;
}

const std::unordered_map<ast::Expr *, Type *>& TypeInfo::types() const {
    return types_;
}

const std::unordered_map<ast::Expr *, constant::Value>& TypeInfo::constant_values() const {
    return constant_values_;
}

const std::unordered_map<ast::Ident *, Object *>& TypeInfo::definitions() const {
    return definitions_;
}

const std::unordered_map<ast::Ident *, Object *>& TypeInfo::uses() const {
    return uses_;
}

const std::unordered_map<ast::Node *, Object *>& TypeInfo::implicits() const {
    return implicits_;
}

const std::unordered_map<ast::Node *, Scope *>& TypeInfo::scopes() const {
    return scopes_;
}

Scope * TypeInfo::universe() const {
    return universe_;
}

Object * TypeInfo::ObjectOf(ast::Ident *ident) const {
    auto defs_it = definitions_.find(ident);
    if (defs_it != definitions_.end()) {
        return defs_it->second;
    }
    auto uses_it = uses_.find(ident);
    if (uses_it != uses_.end()) {
        return uses_it->second;
    }
    return nullptr;
}

Object * TypeInfo::DefinitionOf(ast::Ident *ident) const {
    auto it = definitions_.find(ident);
    if (it != definitions_.end()) {
        return it->second;
    }
    return nullptr;
}

Object * TypeInfo::UseOf(ast::Ident *ident) const {
    auto it = uses_.find(ident);
    if (it != uses_.end()) {
        return it->second;
    }
    return nullptr;
}

Object * TypeInfo::ImplicitOf(ast::Node *node) const {
    auto it = implicits_.find(node);
    if (it != implicits_.end()) {
        return it->second;
    }
    return nullptr;
}

Scope * TypeInfo::ScopeOf(ast::Node *node) const {
    auto it = scopes_.find(node);
    if (it != scopes_.end()) {
        return it->second;
    }
    return nullptr;
}

Type * TypeInfo::TypeOf(ast::Expr *expr) const {
    auto it = types_.find(expr);
    if (it != types_.end()) {
        return it->second;
    }
    if (ast::Ident *ident = dynamic_cast<ast::Ident *>(expr)) {
        Object *ident_obj = ObjectOf(ident);
        if (ident_obj) {
            return ident_obj->type();
        }
    }
    return nullptr;
}

}
}

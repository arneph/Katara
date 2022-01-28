//
//  types.cc
//  Katara
//
//  Created by Arne Philipeit on 7/25/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "types.h"

#include "src/common/logging/logging.h"
#include "src/lang/representation/types/objects.h"

namespace lang {
namespace types {

bool Type::is_wrapper() const {
  TypeKind kind = type_kind();
  return TypeKind::kWrapperStart <= kind && kind <= TypeKind::kWrapperEnd;
}

bool Type::is_container() const {
  TypeKind kind = type_kind();
  return TypeKind::kContainerStart <= kind && kind <= TypeKind::kContainerEnd;
}

std::string Basic::ToString(StringRep) const {
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
      common::fail("unexpected Basic::Kind");
  }
}

std::string Pointer::ToString(StringRep rep) const {
  switch (kind_) {
    case Kind::kStrong:
      return "*" + element_type_->ToString(rep);
    case Kind::kWeak:
      return "%" + element_type_->ToString(rep);
  }
}

std::string Array::ToString(StringRep rep) const {
  return "[" + std::to_string(length()) + "]" + element_type_->ToString(rep);
}

std::string Slice::ToString(StringRep rep) const { return "[]" + element_type_->ToString(rep); }

std::string TypeParameter::ToString(StringRep rep) const {
  switch (rep) {
    case StringRep::kShort:
      return name_;
    case StringRep::kExpanded:
      return name_ + " " + interface_->ToString(StringRep::kExpanded);
  }
}

Type* NamedType::InstanceForTypeArgs(const std::vector<Type*>& type_args) const {
  if (type_parameters_.empty()) {
    common::fail("attempted to access instance of named type without type parameters");
  }
  if (type_args.size() != type_parameters_.size()) {
    common::fail("unexpected number of type arguments for instance");
  }
  for (auto& [instance_type_args, instance] : instances_) {
    bool match = true;
    for (size_t j = 0; j < type_args.size(); j++) {
      Type* type_arg = type_args.at(j);
      Type* instance_type_arg = instance_type_args.at(j);
      if (type_arg != instance_type_arg) {
        match = false;
        break;
      }
    }
    if (match) {
      return instance;
    }
  }
  return nullptr;
}

void NamedType::SetInstanceForTypeArgs(const std::vector<Type*>& type_args, Type* instance) {
  if (InstanceForTypeArgs(type_args) != nullptr) {
    common::fail("attempted to set named type instance for type arguments twice");
  }
  instances_.push_back({type_args, instance});
}

std::string NamedType::ToString(StringRep rep) const {
  std::string s;
  if (is_alias_) {
    s += "=";
  }
  s += name_;
  if (!type_parameters_.empty()) {
    s += "<";
    for (size_t i = 0; i < type_parameters_.size(); i++) {
      if (i > 0) s += ", ";
      s += type_parameters_.at(i)->ToString(rep);
    }
    s += ">";
  }
  return s;
}

std::string TypeInstance::ToString(StringRep) const {
  std::string s = instantiated_type_->name() + "<";
  for (size_t i = 0; i < type_args_.size(); i++) {
    if (i > 0) s += ", ";
    s += type_args_.at(i)->ToString(StringRep::kShort);
  }
  s += ">";
  return s;
}

std::string Tuple::ToString(StringRep rep) const {
  std::string s;
  for (size_t i = 0; i < variables_.size(); i++) {
    if (i > 0) s += ", ";
    std::string name = variables_.at(i)->name();
    if (!name.empty()) {
      s += name + " ";
    }
    s += variables_.at(i)->type()->ToString(rep);
  }
  return s;
}

std::string Signature::ToString(StringRep) const {
  std::string s = "func ";
  if (expr_receiver_ != nullptr) {
    s += "(" + expr_receiver_->ToString() + ") ";
  } else if (type_receiver_ != nullptr) {
    s += "<" + type_receiver_->ToString(StringRep::kShort) + "> ";
  }
  if (!type_parameters_.empty()) {
    s += "<";
    for (size_t i = 0; i < type_parameters_.size(); i++) {
      if (i > 0) s += ", ";
      s += type_parameters_.at(i)->ToString(StringRep::kShort);
    }
    s += ">";
  }
  s += "(";
  if (parameters_ != nullptr) {
    s += parameters_->ToString(StringRep::kShort);
  }
  s += ")";
  if (results_) {
    s += " ";
    if (results_->variables().size() == 1 && results_->variables().at(0)->name().empty()) {
      s += results_->ToString(StringRep::kShort);
    } else {
      s += "(" + results_->ToString(StringRep::kShort) + ")";
    }
  }
  return s;
}

std::string Struct::ToString(StringRep rep) const {
  std::string s = "struct{";
  for (size_t i = 0; i < fields_.size(); i++) {
    if (i > 0) s += "; ";
    std::string name = fields_.at(i)->name();
    if (!name.empty()) {
      s += name + " ";
    }
    s += fields_.at(i)->type()->ToString(rep);
  }
  s += "}";
  return s;
}

std::string Interface::ToString(StringRep rep) const {
  std::string s = "interface{";
  switch (rep) {
    case StringRep::kShort:
      if (!embedded_interfaces_.empty() || !methods_.empty()) {
        s += "...";
      }
      break;
    case StringRep::kExpanded:
      for (size_t i = 0; i < embedded_interfaces_.size(); i++) {
        if (i > 0) s += "; ";
        s += embedded_interfaces_.at(i)->ToString(StringRep::kShort);
      }
      for (size_t i = 0; i < methods_.size(); i++) {
        if (i > 0 || !embedded_interfaces_.empty()) s += "; ";
        s += methods_.at(i)->ToString();
      }
      break;
  }
  s += "}";
  return s;
}

}  // namespace types
}  // namespace lang

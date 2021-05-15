//
//  types_util.cc
//  Katara
//
//  Created by Arne Philipeit on 9/27/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "types_util.h"

#include "lang/representation/types/objects.h"

namespace lang {
namespace types {

Basic::Kind ConvertIfUntyped(Basic::Kind basic_kind) {
  switch (basic_kind) {
    case Basic::kBool:
    case Basic::kInt:
    case Basic::kInt8:
    case Basic::kInt16:
    case Basic::kInt32:
    case Basic::kInt64:
    case Basic::kUint:
    case Basic::kUint8:
    case Basic::kUint16:
    case Basic::kUint32:
    case Basic::kUint64:
    case Basic::kString:
      return basic_kind;
    case Basic::kUntypedBool:
      return Basic::kBool;
    case Basic::kUntypedInt:
      return Basic::kInt;
    case Basic::kUntypedRune:
      return Basic::kRune;
    case Basic::kUntypedString:
      return Basic::kString;
    default:
      throw "internal error: unexpected basic kind";
  }
}

constants::Value ConvertUntypedValue(constants::Value value, Basic::Kind typed_basic_kind) {
  switch (typed_basic_kind) {
    case Basic::kBool:
      return constants::Convert<bool>(value);
    case Basic::kInt8:
      return constants::Convert<int8_t>(value);
    case Basic::kInt16:
      return constants::Convert<int16_t>(value);
    case Basic::kInt32:
      return constants::Convert<int32_t>(value);
    case Basic::kInt64:
    case Basic::kInt:
      return constants::Convert<int64_t>(value);
    case Basic::kUint8:
      return constants::Convert<uint8_t>(value);
    case Basic::kUint16:
      return constants::Convert<uint16_t>(value);
    case Basic::kUint32:
      return constants::Convert<uint32_t>(value);
    case Basic::kUint64:
    case Basic::kUint:
      return constants::Convert<uint16_t>(value);
    case Basic::kString:
      return constants::Convert<std::string>(value);
    default:
      throw "internal error: unexpected typed basic kind";
  }
}

Type* UnderlyingOf(Type* type, InfoBuilder& info_builder) {
  switch (type->type_kind()) {
    case TypeKind::kBasic:
    case TypeKind::kPointer:
    case TypeKind::kArray:
    case TypeKind::kSlice:
    case TypeKind::kTuple:
    case TypeKind::kSignature:
    case TypeKind::kStruct:
    case TypeKind::kInterface:
      return type;
    case TypeKind::kTypeParameter:
      return static_cast<TypeParameter*>(type)->interface();
    case TypeKind::kNamedType:
      return static_cast<NamedType*>(type)->underlying();
    case TypeKind::kTypeInstance: {
      TypeInstance* type_instance = static_cast<TypeInstance*>(type);
      NamedType* instantiated_type = type_instance->instantiated_type();
      if (instantiated_type->type_parameters().empty()) {
        return instantiated_type->underlying();
      }
      const std::vector<Type*>& type_args = type_instance->type_args();
      Type* underlying = instantiated_type->InstanceForTypeArgs(type_args);
      if (underlying == nullptr) {
        InfoBuilder::TypeParamsToArgsMap type_params_to_args;
        type_params_to_args.reserve(type_args.size());
        for (size_t i = 0; i < type_args.size(); i++) {
          TypeParameter* type_param = instantiated_type->type_parameters().at(i);
          Type* type_arg = type_args.at(i);
          type_params_to_args.insert({type_param, type_arg});
        }
        underlying =
            info_builder.InstantiateType(instantiated_type->underlying(), type_params_to_args);
        info_builder.AddInstanceToNamedType(instantiated_type, type_args, underlying);
      }
      return underlying;
    }
    default:
      throw "unexpected lang type";
  }
}

Type* ResolveAlias(Type* type) {
  if (type->type_kind() != TypeKind::kNamedType) {
    return type;
  }
  NamedType* named_type = static_cast<NamedType*>(type);
  if (!named_type->is_alias()) {
    return type;
  }
  return named_type->underlying();
}

bool IsIdentical(Type* a, Type* b) {
  if (a == nullptr || b == nullptr) {
    throw "internal error: attempted to determine identity with nullptr types";
  }
  a = ResolveAlias(a);
  b = ResolveAlias(b);
  if (a == b) {
    return true;
  }
  if (a->type_kind() == TypeKind::kTypeParameter && b->type_kind() == TypeKind::kInterface) {
    return IsIdentical(static_cast<TypeParameter*>(a)->interface(), static_cast<Interface*>(b));
  } else if (a->type_kind() == TypeKind::kInterface && b->type_kind() == TypeKind::kTypeParameter) {
    return IsIdentical(static_cast<Interface*>(a), static_cast<TypeParameter*>(b)->interface());
  }
  if (a->type_kind() != b->type_kind()) {
    return false;
  }
  switch (a->type_kind()) {
    case TypeKind::kBasic:
      return IsIdentical(static_cast<Basic*>(a), static_cast<Basic*>(b));
    case TypeKind::kPointer:
      return IsIdentical(static_cast<Pointer*>(a), static_cast<Pointer*>(b));
    case TypeKind::kArray:
      return IsIdentical(static_cast<Array*>(a), static_cast<Array*>(b));
    case TypeKind::kSlice:
      return IsIdentical(static_cast<Slice*>(a), static_cast<Slice*>(b));
    case TypeKind::kTypeParameter:
      return IsIdentical(static_cast<TypeParameter*>(a), static_cast<TypeParameter*>(b));
    case TypeKind::kNamedType:
      return IsIdentical(static_cast<NamedType*>(a), static_cast<NamedType*>(b));
    case TypeKind::kTypeInstance:
      return IsIdentical(static_cast<TypeInstance*>(a), static_cast<TypeInstance*>(b));
    case TypeKind::kTuple:
      return IsIdentical(static_cast<Tuple*>(a), static_cast<Tuple*>(b));
    case TypeKind::kSignature:
      return IsIdentical(static_cast<Signature*>(a), static_cast<Signature*>(b));
    case TypeKind::kStruct:
      return IsIdentical(static_cast<Struct*>(a), static_cast<Struct*>(b));
    case TypeKind::kInterface:
      return IsIdentical(static_cast<Interface*>(a), static_cast<Interface*>(b));
    default:
      throw "unexpected lang type";
  }
}

bool IsIdentical(Basic* a, Basic* b) { return a->kind() == b->kind(); }

bool IsIdentical(Pointer* a, Pointer* b) {
  if (a->kind() != b->kind()) {
    return false;
  }
  return IsIdentical(a->element_type(), b->element_type());
}

bool IsIdentical(Array* a, Array* b) {
  if (a->length() != b->length()) {
    return false;
  }
  return IsIdentical(a->element_type(), b->element_type());
}

bool IsIdentical(Slice* a, Slice* b) { return IsIdentical(a->element_type(), b->element_type()); }

bool IsIdentical(TypeParameter* a, TypeParameter* b) {
  if (a == nullptr || b == nullptr) {
    throw "internal error: attempted to determine identity with nullptr type parameters";
  }
  return a == b;
}

bool IsIdentical(NamedType* a, NamedType* b) {
  if (a == nullptr || b == nullptr) {
    throw "internal error: attempted to determine identity with nullptr named types";
  }
  return a == b;
}

bool IsIdentical(TypeInstance* a, TypeInstance* b) {
  if (!IsIdentical(a->instantiated_type(), b->instantiated_type())) {
    return false;
  } else if (a->type_args().size() != b->type_args().size()) {
    return false;
  }
  for (size_t i = 0; i < a->type_args().size(); i++) {
    if (!IsIdentical(a->type_args().at(i), b->type_args().at(i))) {
      return false;
    }
  }
  return true;
}

bool IsIdentical(Tuple* a, Tuple* b) {
  if (a->variables().size() != b->variables().size()) {
    return false;
  }
  for (size_t i = 0; i < a->variables().size(); i++) {
    if (!IsIdentical(a->variables().at(i)->type(), b->variables().at(i)->type())) {
      return false;
    }
  }
  return true;
}

bool IsIdentical(Signature* a, Signature* b) {
  if (a->has_expr_receiver() != b->has_expr_receiver() ||
      a->has_type_receiver() != b->has_type_receiver() ||
      a->type_parameters().size() != b->type_parameters().size()) {
    return false;
  }
  if (a->has_expr_receiver() &&
      !IsIdentical(a->expr_receiver()->type(), b->expr_receiver()->type())) {
    return false;
  } else if (a->has_type_receiver() && !IsIdentical(a->type_receiver(), b->type_receiver())) {
    return false;
  }
  for (size_t i = 0; i < a->type_parameters().size(); i++) {
    if (!IsIdentical(a->type_parameters().at(i), b->type_parameters().at(i))) {
      return false;
    }
  }
  if (!IsIdentical(a->parameters(), b->parameters())) {
    return false;
  }
  if ((a->results() == nullptr) != (b->results() == nullptr)) {
    return false;
  } else if (a->results() != nullptr && !IsIdentical(a->results(), b->results())) {
    return false;
  }
  return true;
}

bool IsIdentical(Struct* a, Struct* b) {
  if (a->fields().size() != b->fields().size()) {
    return false;
  }
  for (size_t i = 0; i < a->fields().size(); i++) {
    Variable* field_a = a->fields().at(i);
    Variable* field_b = b->fields().at(i);
    if (field_a->is_embedded() != field_b->is_embedded() || field_a->name() != field_b->name() ||
        field_a->package() != field_b->package() ||
        !IsIdentical(field_a->type(), field_b->type())) {
      return false;
    }
  }
  return true;
}

bool IsIdentical(Interface* a, Interface* b) {
  if (a == b) {
    return true;
  }
  // TODO: handle embedded interfaces
  if (a->methods().size() != b->methods().size()) {
    return false;
  }
  for (size_t i = 0; i < a->methods().size(); i++) {
    Func* method_a = a->methods().at(i);
    Func* method_b = b->methods().at(i);
    if (method_a->name() != method_b->name() || method_a->package() != method_b->package() ||
        !IsIdentical(method_a->type(), method_b->type())) {
      return false;
    }
  }
  return true;
}

bool IsAssignableTo(Type* src, Type* dst, InfoBuilder& info_builder) {
  if (IsIdentical(src, dst)) {
    return true;
  }
  Type* src_underlying = UnderlyingOf(src, info_builder);
  Type* dst_underlying = UnderlyingOf(dst, info_builder);
  if (src->type_kind() != TypeKind::kTypeParameter && src->type_kind() != TypeKind::kNamedType &&
      src->type_kind() != TypeKind::kTypeInstance) {
    if (IsIdentical(src_underlying, dst_underlying)) {
      return true;
    }
  } else if (dst->type_kind() != TypeKind::kTypeParameter &&
             dst->type_kind() != TypeKind::kNamedType &&
             dst->type_kind() != TypeKind::kTypeInstance) {
    if (IsIdentical(src_underlying, dst_underlying)) {
      return true;
    }
  }
  if (Implements(src, dst, info_builder)) {
    return true;
  }

  if (src->type_kind() != TypeKind::kBasic) {
    return false;
  }
  Basic* basic_src = static_cast<Basic*>(src);
  if (basic_src->kind() == Basic::kUntypedNil) {
    switch (dst->type_kind()) {
      case TypeKind::kPointer:
      case TypeKind::kSlice:
      case TypeKind::kTypeParameter:
      case TypeKind::kSignature:
      case TypeKind::kInterface:
        return true;
      default:
        return false;
    }
  } else if ((basic_src->info() & Basic::kIsUntyped) != 0 && dst->type_kind() == TypeKind::kBasic) {
    Basic* basic_dst = static_cast<Basic*>(dst);
    switch (basic_src->kind()) {
      case Basic::kUntypedBool:
        return basic_dst->info() & Basic::kIsBoolean;
      case Basic::kUntypedInt:
      case Basic::kUntypedRune:
        return basic_dst->info() & Basic::kIsInteger;
      case Basic::kUntypedString:
        return basic_dst->info() & Basic::kIsString;
      default:
        throw "internal error: unexpected untyped basic kind";
    }
  }
  return false;
}

bool IsComparable(Type*, Type*) {
  // TODO: implement
  return true;
}

bool IsOrderable(Type*, Type*) {
  // TODO: implement
  return true;
}

bool IsConvertibleTo(Type*, Type*) {
  // TODO: implement
  return true;
}

bool Implements(Type* impl, Type* interface, InfoBuilder& info_builder) {
  Interface* underlying_interface;
  if (Type* underlying = UnderlyingOf(interface, info_builder);
      underlying->type_kind() == TypeKind::kInterface) {
    underlying_interface = static_cast<Interface*>(underlying);
  } else {
    return false;
  }
  if (underlying_interface->is_empty()) {
    return true;
  }
  switch (impl->type_kind()) {
    case TypeKind::kTypeParameter:
      return Implements(static_cast<TypeParameter*>(impl), underlying_interface);
    case TypeKind::kNamedType:
      return Implements(static_cast<NamedType*>(impl), underlying_interface);
    case TypeKind::kTypeInstance:
      return Implements(static_cast<TypeInstance*>(impl), underlying_interface);
    case TypeKind::kInterface:
      return Implements(static_cast<Interface*>(impl), underlying_interface);
    default:
      return false;
  }
}

bool Implements(TypeParameter* impl, Interface* interface) {
  return Implements(impl->interface(), interface);
}

bool Implements(NamedType*, Interface*) {
  // TODO: implement
  return true;
}

bool Implements(TypeInstance*, Interface*) {
  // TODO: implement
  return true;
}

bool Implements(Interface* impl, Interface* interface) {
  if (interface->is_empty() || IsIdentical(impl, interface)) {
    return true;
  }
  // TODO: implement
  return false;
}

bool IsAssertableTo(Type*, Type*) {
  // TODO: implement
  return true;
}

}  // namespace types
}  // namespace lang

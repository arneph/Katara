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

Type * UnderlyingOf(Type *type) {
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
            return static_cast<TypeParameter *>(type)->interface();
        case TypeKind::kNamedType:
            return static_cast<NamedType *>(type)->underlying();
        case TypeKind::kTypeInstance:
            return nullptr;
    }
}

Type * ResolveAlias(Type *type) {
    if (type->type_kind() != TypeKind::kNamedType) {
        return type;
    }
    NamedType *named_type = static_cast<NamedType *>(type);
    if (!named_type->is_alias()) {
        return type;
    }
    return named_type->underlying();
}
    
bool IsIdentical(Type* a, Type *b) {
    if (a == nullptr || b == nullptr) {
        throw "internal error: attempted to determine identity with nullptr types";
    }
    a = ResolveAlias(a);
    b = ResolveAlias(b);
    if (a == b) {
        return true;
    }
    if (a->type_kind() != b->type_kind()) {
        return false;
    }
    switch (a->type_kind()) {
        case TypeKind::kBasic:
            return IsIdentical(static_cast<Basic *>(a),
                               static_cast<Basic *>(b));
        case TypeKind::kPointer:
            return IsIdentical(static_cast<Pointer *>(a),
                               static_cast<Pointer *>(b));
        case TypeKind::kArray:
            return IsIdentical(static_cast<Array *>(a),
                               static_cast<Array *>(b));
        case TypeKind::kSlice:
            return IsIdentical(static_cast<Slice *>(a),
                               static_cast<Slice *>(b));
        case TypeKind::kTypeParameter:
            return IsIdentical(static_cast<TypeParameter *>(a),
                               static_cast<TypeParameter *>(b));
        case TypeKind::kNamedType:
            return IsIdentical(static_cast<NamedType *>(a),
                               static_cast<NamedType *>(b));
        case TypeKind::kTypeInstance:
            return IsIdentical(static_cast<TypeInstance *>(a),
                               static_cast<TypeInstance *>(b));
        case TypeKind::kTuple:
            return IsIdentical(static_cast<Tuple *>(a),
                               static_cast<Tuple *>(b));
        case TypeKind::kSignature:
            return IsIdentical(static_cast<Signature *>(a),
                               static_cast<Signature *>(b));
        case TypeKind::kStruct:
            return IsIdentical(static_cast<Struct *>(a),
                               static_cast<Struct *>(b));
        case TypeKind::kInterface:
            return IsIdentical(static_cast<Interface *>(a),
                               static_cast<Interface *>(b));
    }
}

bool IsIdentical(Basic *a, Basic *b) {
    return a->kind() == b->kind();
}

bool IsIdentical(Pointer *a, Pointer *b) {
    if (a->kind() != b->kind()) {
        return false;
    }
    return IsIdentical(a->element_type(), b->element_type());
}

bool IsIdentical(Array *a, Array *b) {
    if (a->length() != b->length()) {
        return false;
    }
    return IsIdentical(a->element_type(), b->element_type());
}

bool IsIdentical(Slice *a, Slice *b) {
    return IsIdentical(a->element_type(), b->element_type());
}

bool IsIdentical(TypeParameter *a, TypeParameter *b) {
    if (a == nullptr || b == nullptr) {
        throw "internal error: attempted to determine identity with nullptr type parameters";
    }
    return a == b;
}

bool IsIdentical(NamedType *a, NamedType *b) {
    if (a == nullptr || b == nullptr) {
        throw "internal error: attempted to determine identity with nullptr named types";
    }
    return a == b;
}

bool IsIdentical(TypeInstance *a, TypeInstance *b) {
    if (!IsIdentical(a->instantiated_type(), b->instantiated_type())) {
        return false;
    } else if (a->type_args().size() != b->type_args().size()) {
        return false;
    }
    for (int i = 0; i < a->type_args().size(); i++) {
        if (!IsIdentical(a->type_args().at(i), b->type_args().at(i))) {
            return false;
        }
    }
    return true;
}

bool IsIdentical(Tuple *a, Tuple *b) {
    if (a->variables().size() != b->variables().size()) {
        return false;
    }
    for (int i = 0; i < a->variables().size(); i++) {
        if (!IsIdentical(a->variables().at(i)->type(), b->variables().at(i)->type())) {
            return false;
        }
    }
    return true;
}

bool IsIdentical(Signature *a, Signature *b) {
    if (a->has_expr_receiver() != b->has_expr_receiver() ||
        a->has_type_receiver() != b->has_type_receiver() ||
        a->type_parameters().size() != b->type_parameters().size()) {
        return false;
    }
    if (a->has_expr_receiver() &&
        !IsIdentical(a->expr_receiver()->type(), b->expr_receiver()->type())) {
        return false;
    } else if (a->has_type_receiver() &&
               !IsIdentical(a->type_receiver(), b->type_receiver())) {
        return false;
    }
    for (int i = 0; i < a->type_parameters().size(); i++) {
        if (!IsIdentical(a->type_parameters().at(i), b->type_parameters().at(i))) {
            return false;
        }
    }
    if (!IsIdentical(a->parameters(), b->parameters())) {
        return false;
    }
    if (!IsIdentical(a->results(), b->results())) {
        return false;
    }
    return true;
}

bool IsIdentical(Struct *a, Struct *b) {
    if (a->fields().size() != b->fields().size()) {
        return false;
    }
    for (int i = 0; i < a->fields().size(); i++) {
        Variable *field_a = a->fields().at(i);
        Variable *field_b = b->fields().at(i);
        if (field_a->is_embedded() != field_b->is_embedded() ||
            field_a->name() != field_b->name() ||
            field_a->package() != field_b->package() ||
            !IsIdentical(field_a->type(), field_b->type())) {
            return false;
        }
    }
    return true;
}

bool IsIdentical(Interface *a, Interface *b) {
    // TODO: handle embedded interfaces
    if (a->methods().size() != b->embedded_interfaces().size()) {
        return false;
    }
    for (int i = 0; i < a->methods().size(); i++) {
        Func *method_a = a->methods().at(i);
        Func *method_b = b->methods().at(i);
        if (method_a->name() != method_b->name() ||
            method_a->package() != method_b->package() ||
            !IsIdentical(method_a->type(), method_b->type())) {
            return false;
        }
    }
    return true;
}

bool IsAssignableTo(Type *src, Type *dst) {
    if (IsIdentical(src, dst)) {
        return true;
    }
    if (src->type_kind() == TypeKind::kNamedType &&
        dst->type_kind() != TypeKind::kNamedType &&
        IsIdentical(static_cast<NamedType *>(src)->underlying(), dst)) {
        return true;
    } else if (src->type_kind() != TypeKind::kNamedType &&
               dst->type_kind() == TypeKind::kNamedType &&
               IsIdentical(src, static_cast<NamedType *>(dst)->underlying())) {
        return true;
    }
    if (dst->type_kind() == TypeKind::kInterface &&
        Implements(src, static_cast<Interface *>(dst))) {
        return true;
    } else if (dst->type_kind() == TypeKind::kTypeParameter &&
               Implements(src, static_cast<TypeParameter *>(dst)->interface())) {
        return true;
    }
    
    if (src->type_kind() != TypeKind::kBasic) {
        return false;
    }
    Basic *basic_src = static_cast<Basic *>(src);
    if (dst->type_kind() == TypeKind::kTypeInstance) {
        dst = static_cast<TypeInstance *>(dst)->instantiated_type()->underlying();
    } else if (dst) {
        dst = UnderlyingOf(dst);
    }
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
    } else if ((basic_src->info() & Basic::kIsUntyped) != 0 &&
               dst->type_kind() == TypeKind::kBasic) {
        Basic *basic_dst = static_cast<Basic *>(dst);
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

bool IsComparable(Type *t, Type *v) {
    // TODO: implement
    return true;
}

bool IsOrderable(Type *t, Type *v) {
    // TODO: implement
    return true;
}

bool IsConvertibleTo(Type *src, Type *dst) {
    // TODO: implement
    return true;
}

bool Implements(Type *impl, Interface *interface) {
    if (interface->is_empty()) {
        return true;
    }
    switch (impl->type_kind()) {
        case TypeKind::kTypeParameter:
            return Implements(static_cast<TypeParameter *>(impl),
                              interface);
        case TypeKind::kNamedType:
            return Implements(static_cast<NamedType *>(impl),
                              interface);
        case TypeKind::kTypeInstance:
            return Implements(static_cast<TypeInstance *>(impl),
                              interface);
        case TypeKind::kInterface:
            return Implements(static_cast<Interface *>(impl),
                              interface);
        default:
            return false;
    }
}

bool Implements(TypeParameter *impl, Interface *interface) {
    return Implements(impl->interface(), interface);
}

bool Implements(NamedType *impl, Interface *interface) {
    // TODO: implement
    return true;
}

bool Implements(TypeInstance *impl, Interface *interface) {
    // TODO: implement
    return true;
}

bool Implements(Interface *impl, Interface *interface) {
    if (interface->is_empty() ||
        IsIdentical(impl, interface)) {
        return true;
    }
    // TODO: implement
    return false;
}

bool IsAssertableTo(Type *general, Type *specialised) {
    // TODO: implement
    return true;
}

}
}

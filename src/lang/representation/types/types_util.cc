//
//  types_util.cc
//  Katara
//
//  Created by Arne Philipeit on 9/27/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "types_util.h"


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

bool IsAssignableTo(Type *src, Type *dst) {
    // TODO: implement
    return true;
}

bool IsAssertableTo(Type *general, Type *specialised) {
    // TODO: implement
    return true;
}

bool IsComparable(Type *t) {
    // TODO: implement
    return true;
}

bool IsComparable(Type *t, Type *v, tokens::Token op) {
    // TODO: implement
    return true;
}

bool IsConvertibleTo(Type *src, Type *dst) {
    // TODO: implement
    return true;
}

}
}

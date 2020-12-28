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
    if (dynamic_cast<Basic *>(type) != nullptr ||
        dynamic_cast<Wrapper *>(type) != nullptr ||
        dynamic_cast<Tuple *>(type) != nullptr ||
        dynamic_cast<Signature *>(type) != nullptr ||
        dynamic_cast<Struct *>(type) != nullptr ||
        dynamic_cast<Interface *>(type) != nullptr) {
        return type;
    }
    if (TypeParameter *type_parameter = dynamic_cast<TypeParameter *>(type)) {
        return type_parameter->interface();
    } else if (NamedType * named_type = dynamic_cast<NamedType *>(type)) {
        return named_type->underlying();
    } else {
        // Note: TypeInstance has no defined underlying type.
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

//
//  types_util.h
//  Katara
//
//  Created by Arne Philipeit on 9/27/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_util_h
#define lang_types_util_h

#include "lang/representation/constants/constants.h"
#include "lang/representation/tokens/tokens.h"
#include "lang/representation/types/types.h"

namespace lang {
namespace types {

// Converts the given Basic::Kind to its typed equivalent if untyped, otherwise returns the already
// typed input, e.g. for
// var a = 5
// it converts from untyped int, the type of "5", to int, the type of "a". Conversion of untyped nil
// is not possible.
Basic::Kind ConvertIfUntyped(Basic::Kind untyped_basic_kind);

// Converts the given constants::Value of an untyped Basic::Kind to its typed equivalent, e.g. for
// var x int8 = 17
// const y = uint32(42)
// it converts from untyped int, the type of "17" and "42", to int8 or uint32.
constants::Value ConvertUntypedValue(constants::Value value, Basic::Kind typed_basic_kind);

Type* UnderlyingOf(Type* type);

Type* ResolveAlias(Type* type);

bool IsIdentical(Type* a, Type* b);
bool IsIdentical(Basic* a, Basic* b);
bool IsIdentical(Pointer* a, Pointer* b);
bool IsIdentical(Array* a, Array* b);
bool IsIdentical(Slice* a, Slice* b);
bool IsIdentical(TypeParameter* a, TypeParameter* b);
bool IsIdentical(NamedType* a, NamedType* b);
bool IsIdentical(TypeInstance* a, TypeInstance* b);
bool IsIdentical(Tuple* a, Tuple* b);
bool IsIdentical(Signature* a, Signature* b);
bool IsIdentical(Struct* a, Struct* b);
bool IsIdentical(Interface* a, Interface* b);

bool IsAssignableTo(Type* src, Type* dst);
bool IsComparable(Type* t, Type* v);
bool IsOrderable(Type* t, Type* v);
bool IsConvertibleTo(Type* src, Type* dst);

bool Implements(Type* impl, Interface* interface);
bool Implements(TypeParameter* impl, Interface* interface);
bool Implements(NamedType* impl, Interface* interface);
bool Implements(TypeInstance* impl, Interface* interface);
bool Implements(Interface* impl, Interface* interface);

bool IsAssertableTo(Type* general, Type* specialised);

}  // namespace types
}  // namespace lang

#endif /* types_util_h */

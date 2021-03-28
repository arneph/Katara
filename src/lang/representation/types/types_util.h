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

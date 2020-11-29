//
//  types_util.h
//  Katara
//
//  Created by Arne Philipeit on 9/27/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_util_h
#define lang_types_util_h

#include <string>
#include <unordered_map>

#include "lang/representation/positions/positions.h"
#include "lang/representation/tokens/tokens.h"
#include "lang/representation/types/types.h"

namespace lang {
namespace types {

std::string TypeInfoToText(pos::FileSet *file_set, TypeInfo *type_info);

bool IsAssignableTo(Type *src, Type *dst);
bool IsAssertableTo(Type *general, Type *specialised);
bool IsComparable(Type *t);
bool IsComparable(Type *t, Type *v, tokens::Token op);
bool IsConvertibleTo(Type *src, Type *dst);

Type * InstantiateType(Type *parameterized_type,
                       std::unordered_map<NamedType *, Type *> type_params_to_args,
                       TypeInfo *info);

}
}

#endif /* types_util_h */

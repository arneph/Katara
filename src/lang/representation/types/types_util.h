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

#include "lang/representation/positions/positions.h"
#include "lang/representation/types/types.h"

namespace lang {
namespace types {

std::string TypeInfoToText(pos::FileSet *file_set, TypeInfo *type_info);

bool IsAssignableTo(Type *v, Type *t);
bool IsAssertableTo(Interface *interface, Type *t);

}
}

#endif /* types_util_h */

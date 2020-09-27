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

#include "lang/positions.h"
#include "lang/types.h"

namespace lang {
namespace types {

std::string TypeInfoToText(pos::FileSet *file_set, TypeInfo *type_info);

}
}

#endif /* types_util_h */

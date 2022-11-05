//
//  info_util.h
//  Katara
//
//  Created by Arne Philipeit on 11/29/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#ifndef lang_types_info_util_h
#define lang_types_info_util_h

#include <string>

#include "src/common/positions/positions.h"
#include "src/lang/representation/types/info.h"

namespace lang {
namespace types {

std::string InfoToText(const common::FileSet* file_set, Info* info);

}
}  // namespace lang

#endif /* lang_types_info_util_h */

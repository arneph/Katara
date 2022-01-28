//
//  size_translator.h
//  Katara
//
//  Created by Arne Philipeit on 8/11/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#ifndef ir_to_x86_64_translator_size_translator_h
#define ir_to_x86_64_translator_size_translator_h

#include "src/common/atomics/atomics.h"
#include "src/ir/representation/types.h"
#include "src/x86_64/ops.h"

namespace ir_to_x86_64_translator {

x86_64::Size TranslateSizeOfType(const ir::Type* ir_type);
x86_64::Size TranslateSizeOfIntType(const ir::IntType* ir_int_type);
x86_64::Size TranslateSizeOfIntType(common::IntType common_int_type);

}  // namespace ir_to_x86_64_translator

#endif /* ir_to_x86_64_translator_size_translator_h */

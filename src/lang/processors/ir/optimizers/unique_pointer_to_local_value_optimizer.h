//
//  unique_pointer_to_local_value_optimizer.h
//  Katara
//
//  Created by Arne Philipeit on 5/8/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_optimizers_unique_pointer_to_local_value_optimizer_h
#define lang_ir_optimizers_unique_pointer_to_local_value_optimizer_h

#include "src/ir/representation/program.h"

namespace lang {
namespace ir_optimizers {

void ConvertUniquePointersToLocalValuesInProgram(ir::Program* program);

}
}  // namespace lang

#endif /* lang_ir_optimizers_unique_pointer_to_local_value_optimizer_h */

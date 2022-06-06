//
//  shared_to_unique_pointer_optimizer.h
//  Katara
//
//  Created by Arne Philipeit on 2/19/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#ifndef lang_ir_optimizers_shared_to_unique_pointer_optimizer_h
#define lang_ir_optimizers_shared_to_unique_pointer_optimizer_h

#include "src/ir/representation/program.h"

namespace lang {
namespace ir_optimizers {

void ConvertSharedToUniquePointersInProgram(ir::Program* program);

}
}  // namespace lang

#endif /* lang_ir_optimizers_shared_to_unique_pointer_optimizer_h */
